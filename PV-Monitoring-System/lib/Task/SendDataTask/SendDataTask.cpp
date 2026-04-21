#include "SendDataTask.h"

#include "../../Modules/PV_Sense/PVData.h"
#include "../WifiTask/WifiTask.h"
#include "../dataTask/dataTask.h"

#include <InfluxDbClient.h>
#include <WiFi.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <freertos/queue.h>
#include <time.h>

namespace SendDataTask {

namespace {

Config gConfig;
TaskHandle_t gTaskHandle = nullptr;
MQTTModule::MQTTConfigManager gMqttManager;
InfluxDBClient* gInfluxClient = nullptr;
QueueHandle_t gInfluxQueue = nullptr;
TaskHandle_t gInfluxTaskHandle = nullptr;
bool gInfluxNtpSynced = false;
Status gStatus = Status::Idle;
bool gStarted = false;
bool gLastMqttConnected = false;
bool gStatusPublished = false;
unsigned long gLastPublishMs = 0;
unsigned long gLastStatusMs = 0;

struct InfluxSample {
  PVSense::MpptData pvData{};
  InverterSense::InverterSnapshot inverterData{};
  bool hasInverterData = false;
};

const char* taskStatusToString(Status status) {
  switch (status) {
    case Status::Idle:
      return "Idle";
    case Status::WaitingWifi:
      return "Waiting WiFi";
    case Status::WaitingPVData:
      return "Waiting PV Data";
    case Status::ConnectingBroker:
      return "Connecting Broker";
    case Status::Ready:
      return "Ready";
    case Status::PublishSuccess:
      return "Publish Success";
    case Status::PublishFailed:
      return "Publish Failed";
    default:
      return "Unknown";
  }
}

void setStatus(Status status) {
  gStatus = status;
}

bool influxSettingsValid(const InfluxSettings& s) {
  return s.enabled && s.url != nullptr && s.url[0] != '\0' && s.org != nullptr && s.org[0] != '\0' &&
         s.bucket != nullptr && s.bucket[0] != '\0' && s.token != nullptr && s.token[0] != '\0';
}

void printBanner() {
  if (!gConfig.printEventsToSerial) {
    return;
  }

  Serial.println();
  Serial.println("=====================================");
  Serial.println("          MQTT SendDataTask");
  Serial.println("=====================================");
  gMqttManager.printStatus(Serial);
  Serial.println();
  Serial.println("=====================================");
  Serial.println("          InfluxDB (Arduino client)");
  Serial.println("=====================================");
  if (gInfluxClient != nullptr) {
    Serial.printf("[INFLUX] URL         : %s\n", gConfig.influx.url);
    Serial.printf("[INFLUX] Org         : %s\n", gConfig.influx.org);
    Serial.printf("[INFLUX] Bucket      : %s\n", gConfig.influx.bucket);
    Serial.printf("[INFLUX] Measurement : %s\n", gConfig.influx.measurement);
    Serial.printf("[INFLUX] TZ (NTP)    : %s\n", gConfig.influx.tzInfo);
  } else {
    Serial.println("[INFLUX] Disabled (no URL/token atau enabled=false)");
  }
  Serial.printf("[SEND] Data Interval  : %lu ms\n",
                static_cast<unsigned long>(gConfig.publishIntervalMs));
  Serial.printf("[SEND] Status Interval: %lu ms\n",
                static_cast<unsigned long>(gConfig.statusIntervalMs));
  Serial.println();
}

size_t buildDeviceInfoPayload(char* buffer, size_t bufferSize, Status sendTaskStatus) {
  const long rssi = static_cast<long>(WiFi.RSSI());
  const String ip = WifiTask::getIpAddress();
  const String ssid = WifiTask::getSsid();
  const unsigned long uptimeMs = millis();

  PVSense::MpptData pvProbe{};
  const bool mpptDataOk = DataTask::getLatestPVData(pvProbe);
  (void)mpptDataOk;
  const char* mpptPoll = PVSense::PVData::pollStatusToString(DataTask::getLastPVStatus());
  const char* invRead =
      InverterSense::InverterData::readStatusToString(DataTask::getLastInverterStatus());

  const int written = std::snprintf(
      buffer,
      bufferSize,
      "{\"device_id\":\"%s\","
      "\"firmware_version\":\"%s\","
      "\"wifi_ssid\":\"%s\","
      "\"wifi_rssi\":%ld,"
      "\"ip\":\"%s\","
      "\"uptime_ms\":%lu,"
      "\"mqtt\":\"connected\","
      "\"pv_sensor_data_ok\":%s,"
      "\"mppt_poll_status\":\"%s\","
      "\"inverter_read_status\":\"%s\","
      "\"send_task_status\":\"%s\","
      "\"data_topic\":\"%s\","
      "\"status\":\"online\"}",
      gMqttManager.getDeviceId(),
      gConfig.firmwareVersion,
      ssid.c_str(),
      rssi,
      ip.c_str(),
      uptimeMs,
      mpptDataOk ? "true" : "false",
      mpptPoll,
      invRead,
      taskStatusToString(sendTaskStatus),
      gMqttManager.getPubsTopic());

  if (written <= 0 || written >= static_cast<int>(bufferSize)) {
    return 0;
  }
  return static_cast<size_t>(written);
}

bool publishDeviceInfoToMqtt(Status sendTaskStatus) {
  char payload[768] = {};
  if (buildDeviceInfoPayload(payload, sizeof(payload), sendTaskStatus) == 0U) {
    return false;
  }
  return gMqttManager.publishDeviceInfo(payload);
}

size_t buildDataPayload(char* buffer,
                        size_t bufferSize,
                        const PVSense::MpptData& pvData,
                        const InverterSense::InverterSnapshot& inverterData,
                        bool hasInverterData) {
  const int written = std::snprintf(
      buffer,
      bufferSize,
      "{\"device_id\":\"%s\","
      "\"mppt_valid\":true,"
      "\"mppt_pv_voltage\":%.1f,"
      "\"mppt_charging_power\":%.1f,"
      "\"mppt_charging_current\":%.2f,"
      "\"mppt_battery_voltage\":%.1f,"
      "\"mppt_charging_status\":%u,"
      "\"mppt_charging_status_text\":\"%s\","
      "\"mppt_load_current\":%.2f,"
      "\"mppt_load_power\":%.1f,"
      "\"mppt_fault_code\":%u,"
      "\"mppt_timestamp_ms\":%lu,"
      "\"inverter_valid\":%s,"
      "\"inverter_ac_voltage\":%.1f,"
      "\"inverter_ac_current\":%.3f,"
      "\"inverter_ac_power\":%.1f,"
      "\"inverter_ac_energy\":%.3f,"
      "\"inverter_ac_frequency\":%.1f,"
      "\"inverter_ac_power_factor\":%.2f,"
      "\"inverter_ac_apparent_power\":%.1f,"
      "\"inverter_timestamp_ms\":%lu,"
      "\"wifi_rssi\":%ld}",
      gMqttManager.getDeviceId(),
      static_cast<double>(pvData.pvVoltage),
      static_cast<double>(pvData.chargingPower),
      static_cast<double>(pvData.chargingCurrent),
      static_cast<double>(pvData.batteryVoltage),
      static_cast<unsigned>(pvData.chargingStatus),
      PVSense::PVData::chargingStatusToString(pvData.chargingStatus),
      static_cast<double>(pvData.loadCurrent),
      static_cast<double>(pvData.loadPower),
      static_cast<unsigned>(pvData.faultCode),
      static_cast<unsigned long>(pvData.timestampMs),
      hasInverterData ? "true" : "false",
      static_cast<double>(hasInverterData ? inverterData.voltage : 0.0f),
      static_cast<double>(hasInverterData ? inverterData.current : 0.0f),
      static_cast<double>(hasInverterData ? inverterData.power : 0.0f),
      static_cast<double>(hasInverterData ? inverterData.energy : 0.0f),
      static_cast<double>(hasInverterData ? inverterData.frequency : 0.0f),
      static_cast<double>(hasInverterData ? inverterData.powerFactor : 0.0f),
      static_cast<double>(hasInverterData ? inverterData.apparentPower : 0.0f),
      static_cast<unsigned long>(hasInverterData ? inverterData.timestampMs : 0UL),
      static_cast<long>(WiFi.RSSI()));

  if (written <= 0 || written >= static_cast<int>(bufferSize)) {
    return 0;
  }
  return static_cast<size_t>(written);
}

Point buildInfluxPoint(const PVSense::MpptData& pvData,
                       const InverterSense::InverterSnapshot& inverterData,
                       bool hasInverterData) {
  const char* meas =
      (gConfig.influx.measurement != nullptr && gConfig.influx.measurement[0] != '\0')
          ? gConfig.influx.measurement
          : "pv_monitoring";

  // Brace-init: `Point p(String(...))` adalah *most vexing parse* (deklarasi fungsi), bukan objek.
  Point point{String(meas)};
  point.addTag("device_id", String(gMqttManager.getDeviceId()));
  point.addTag("charging_status", String(static_cast<unsigned>(pvData.chargingStatus)));

  point.addField("mppt_pv_voltage", static_cast<double>(pvData.pvVoltage));
  point.addField("mppt_charging_power", static_cast<double>(pvData.chargingPower));
  point.addField("mppt_charging_current", static_cast<double>(pvData.chargingCurrent));
  point.addField("mppt_battery_voltage", static_cast<double>(pvData.batteryVoltage));
  point.addField("mppt_load_current", static_cast<double>(pvData.loadCurrent));
  point.addField("mppt_load_power", static_cast<double>(pvData.loadPower));
  point.addField("mppt_fault_code", static_cast<long>(pvData.faultCode));

  point.addField("inverter_valid", hasInverterData);
  point.addField("inverter_ac_voltage", static_cast<double>(hasInverterData ? inverterData.voltage : 0.0f));
  point.addField("inverter_ac_current", static_cast<double>(hasInverterData ? inverterData.current : 0.0f));
  point.addField("inverter_ac_power", static_cast<double>(hasInverterData ? inverterData.power : 0.0f));
  point.addField("inverter_ac_energy", static_cast<double>(hasInverterData ? inverterData.energy : 0.0f));
  point.addField("inverter_ac_frequency", static_cast<double>(hasInverterData ? inverterData.frequency : 0.0f));
  point.addField("inverter_ac_power_factor", static_cast<double>(hasInverterData ? inverterData.powerFactor : 0.0f));
  point.addField("inverter_ac_apparent_power",
                 static_cast<double>(hasInverterData ? inverterData.apparentPower : 0.0f));

  point.addField("wifi_rssi", static_cast<long>(WiFi.RSSI()));
  point.setTime(WritePrecision::MS);
  return point;
}

/// NTP + TZ (ganti `timeSync()` dari Helpers.h — path header tidak konsisten antar versi library).
void syncTimeWithNtp(const char* tzInfo) {
  if (tzInfo != nullptr && tzInfo[0] != '\0') {
    setenv("TZ", tzInfo, 1);
    tzset();
  }
  configTime(0, 0, "pool.ntp.org", "time.nis.gov");
  for (int i = 0; i < 100; ++i) {
    const time_t now = time(nullptr);
    if (now > 1577836800) {  // > 2020-01-01 UTC
      return;
    }
    delay(100);
  }
}

void maybeSyncInfluxTimeAndValidate() {
  if (gInfluxClient == nullptr || !gConfig.influx.enabled || gInfluxNtpSynced) {
    return;
  }
  if (!WifiTask::isConnected()) {
    return;
  }

  gInfluxNtpSynced = true;

  const char* tz = (gConfig.influx.tzInfo != nullptr && gConfig.influx.tzInfo[0] != '\0')
                      ? gConfig.influx.tzInfo
                      : "UTC7";
  syncTimeWithNtp(tz);

  if (gConfig.printEventsToSerial) {
    if (gInfluxClient->validateConnection()) {
      Serial.print("[INFLUX] Connected to InfluxDB: ");
      Serial.println(gInfluxClient->getServerUrl());
    } else {
      Serial.print("[INFLUX] Connection check failed: ");
      Serial.println(gInfluxClient->getLastErrorMessage());
    }
  }
}

void influxTaskLoop(void* parameter) {
  (void)parameter;

  for (;;) {
    if (gInfluxClient == nullptr || !gConfig.influx.enabled || gInfluxQueue == nullptr) {
      vTaskDelay(pdMS_TO_TICKS(1000));
      continue;
    }

    if (!WifiTask::isConnected()) {
      vTaskDelay(pdMS_TO_TICKS(500));
      continue;
    }

    // NTP + validate connection bisa blocking → lakukan di task Influx, bukan task MQTT.
    maybeSyncInfluxTimeAndValidate();

    InfluxSample sample{};
    if (xQueueReceive(gInfluxQueue, &sample, pdMS_TO_TICKS(1000)) != pdTRUE) {
      continue;
    }

    const Point point =
        buildInfluxPoint(sample.pvData, sample.inverterData, sample.hasInverterData);

    // writePoint() berpotensi blocking (HTTP). Ini sengaja dipisahkan dari loop MQTT.
    Point writable = point;
    const bool ok = gInfluxClient->writePoint(writable);
    if (!ok && gConfig.printEventsToSerial) {
      Serial.print("[INFLUX] writePoint failed: ");
      Serial.println(gInfluxClient->getLastErrorMessage());
    }
  }
}

void handleIncomingMessage(const char* topic, const uint8_t* payload, unsigned int length) {
  if (topic == nullptr || std::strcmp(topic, gMqttManager.getSubsTopic()) != 0) {
    return;
  }

  char message[64] = {};
  const size_t copyLength = (length < sizeof(message) - 1U) ? length : sizeof(message) - 1U;
  std::memcpy(message, payload, copyLength);
  message[copyLength] = '\0';

  for (size_t i = copyLength; i > 0U; --i) {
    const char ch = message[i - 1U];
    if (ch == '\r' || ch == '\n' || ch == ' ' || ch == '\t') {
      message[i - 1U] = '\0';
    } else {
      break;
    }
  }

  if (gConfig.printEventsToSerial) {
    Serial.printf("[SUBS] %s <- %s\n", topic, message);
  }

  const bool isPing =
      (std::strcmp(message, "ping") == 0) || (std::strstr(message, "\"ping\"") != nullptr);

  if (isPing) {
    const bool ok = publishDeviceInfoToMqtt(gStatus);
    if (gConfig.printEventsToSerial) {
      Serial.printf("[SUBS] ping -> info publish %s\n", ok ? "OK" : "FAIL");
    }
  }
}

bool publishDataSnapshot(const PVSense::MpptData& pvData) {
  InverterSense::InverterSnapshot inverterData{};
  const bool hasInverterData = DataTask::getLatestInverterData(inverterData);

  bool mqttOk = false;
  {
    char payload[1024] = {};
    if (buildDataPayload(payload, sizeof(payload), pvData, inverterData, hasInverterData) == 0U) {
      return false;
    }
    mqttOk = gMqttManager.publishPubs(payload);
  }

  bool influxQueued = false;
  bool influxDropped = false;
  if (gInfluxClient != nullptr && gConfig.influx.enabled && gInfluxQueue != nullptr) {
    InfluxSample sample{};
    sample.pvData = pvData;
    sample.inverterData = inverterData;
    sample.hasInverterData = hasInverterData;

    // Non-blocking enqueue. Kalau penuh, drop item tertua lalu coba sekali lagi.
    if (xQueueSend(gInfluxQueue, &sample, 0) == pdTRUE) {
      influxQueued = true;
    } else {
      InfluxSample dropped{};
      (void)xQueueReceive(gInfluxQueue, &dropped, 0);
      if (xQueueSend(gInfluxQueue, &sample, 0) == pdTRUE) {
        influxQueued = true;
        influxDropped = true;
      } else {
        influxDropped = true;
      }
    }
  }

  if (gConfig.printEventsToSerial) {
    if (gInfluxClient != nullptr && gConfig.influx.enabled) {
      const char* influxState = (gInfluxQueue == nullptr)
                                    ? "OFF"
                                    : (influxQueued ? (influxDropped ? "QUEUED(drop-old)" : "QUEUED")
                                                    : "DROP(queue-full)");
      Serial.printf("[SEND] MQTT=%s | INFLUX=%s | MPPT[pv=%.1fV batt=%.1fV] | INV[%s]\n",
                    mqttOk ? "OK" : "FAIL",
                    influxState,
                    pvData.pvVoltage,
                    pvData.batteryVoltage,
                    hasInverterData ? "data" : "no data");
    } else {
      Serial.printf("[SEND] MQTT=%s | INFLUX=OFF | MPPT[pv=%.1fV batt=%.1fV] | INV[%s]\n",
                    mqttOk ? "OK" : "FAIL",
                    pvData.pvVoltage,
                    pvData.batteryVoltage,
                    hasInverterData ? "data" : "no data");
    }
  }

  return mqttOk;
}

void printStatusChange(Status currentStatus, bool mqttConnected) {
  if (!gConfig.printEventsToSerial) {
    return;
  }

  Serial.printf("[SEND] Status   : %s\n", taskStatusToString(currentStatus));
  Serial.printf("[SEND] MQTT     : %s\n", mqttConnected ? "Connected" : "Disconnected");

  if (mqttConnected) {
    Serial.printf("[SEND] Broker OK: %s\n", MQTTModule::MQTTConfigManager::clientStateToString(
                                               gMqttManager.getClientState()));
  } else {
    Serial.printf("[SEND] Reason   : %s\n",
                  MQTTModule::MQTTConfigManager::statusToString(gMqttManager.getStatus()));
  }
}

void sendTaskLoop(void* parameter) {
  (void)parameter;

  const TickType_t intervalTicks =
      pdMS_TO_TICKS((gConfig.taskIntervalMs > 0U) ? gConfig.taskIntervalMs : 1U);
  TickType_t lastWakeTime = xTaskGetTickCount();
  Status previousStatus = Status::Idle;

  for (;;) {
    if (!WifiTask::isConnected()) {
      gMqttManager.disconnect();
      gStatusPublished = false;
      gInfluxNtpSynced = false;
      setStatus(Status::WaitingWifi);
    } else if (!gMqttManager.ensureConnected()) {
      setStatus(Status::ConnectingBroker);
    } else {
      gMqttManager.loop();

      const unsigned long now = millis();

      PVSense::MpptData pvData{};
      if (!DataTask::getLatestPVData(pvData)) {
        setStatus(Status::WaitingPVData);
      } else {
        setStatus(Status::Ready);

        if ((gLastPublishMs == 0U) || ((now - gLastPublishMs) >= gConfig.publishIntervalMs)) {
          const bool published = publishDataSnapshot(pvData);
          setStatus(published ? Status::PublishSuccess : Status::PublishFailed);

          if (published) {
            gLastPublishMs = now;
          }
        }
      }

      // Info topic global `pv-monitoring/info` — setelah status PV/ task diperbarui.
      if (!gStatusPublished && gConfig.publishStatusOnConnect) {
        if (publishDeviceInfoToMqtt(gStatus)) {
          gStatusPublished = true;
          gLastStatusMs = now;
          if (gConfig.printEventsToSerial) {
            Serial.println("[SEND] pv-monitoring/info -> MQTT: OK");
          }
        }
      } else if (gConfig.statusIntervalMs > 0U &&
                 (now - gLastStatusMs) >= gConfig.statusIntervalMs) {
        if (publishDeviceInfoToMqtt(gStatus)) {
          gLastStatusMs = now;
        }
      }
    }

    const bool mqttConnected = gMqttManager.isConnected();
    if ((gStatus != previousStatus) || (mqttConnected != gLastMqttConnected)) {
      printStatusChange(gStatus, mqttConnected);
      previousStatus = gStatus;
      gLastMqttConnected = mqttConnected;
    }

    vTaskDelayUntil(&lastWakeTime, (intervalTicks > 0U) ? intervalTicks : 1U);
  }
}

}  // namespace

bool begin(const Config& config) {
  if (gStarted) {
    return true;
  }

  gConfig = config;

  if (!gMqttManager.begin(gConfig.mqttConfig)) {
    return false;
  }

  gMqttManager.setMessageCallback(&handleIncomingMessage);

  if (influxSettingsValid(gConfig.influx)) {
    gInfluxClient = new InfluxDBClient(gConfig.influx.url, gConfig.influx.org, gConfig.influx.bucket,
                                       gConfig.influx.token);
    // tobiasschuerg/ESP8266 Influxdb v3.x — pakai WriteOptions, bukan setWritePrecision.
    gInfluxClient->setWriteOptions(WriteOptions().writePrecision(WritePrecision::MS));

    // Queue + task terpisah supaya Influx (HTTP) tidak nge-block MQTT publish.
    gInfluxQueue = xQueueCreate(10, sizeof(InfluxSample));
    if (gInfluxQueue != nullptr) {
      (void)xTaskCreatePinnedToCore(influxTaskLoop,
                                   "InfluxTask",
                                   8192,
                                   nullptr,
                                   gConfig.priority,
                                   &gInfluxTaskHandle,
                                   0);
    }
  }

  printBanner();

  BaseType_t result = pdFAIL;
  if (gConfig.coreId == tskNO_AFFINITY) {
    result = xTaskCreate(sendTaskLoop,
                         "SendDataTask",
                         gConfig.stackSize,
                         nullptr,
                         gConfig.priority,
                         &gTaskHandle);
  } else {
    result = xTaskCreatePinnedToCore(sendTaskLoop,
                                     "SendDataTask",
                                     gConfig.stackSize,
                                     nullptr,
                                     gConfig.priority,
                                     &gTaskHandle,
                                     gConfig.coreId);
  }

  gStarted = (result == pdPASS);
  return gStarted;
}

bool isConnected() {
  return gMqttManager.isConnected();
}

Status getStatus() {
  return gStatus;
}

MQTTModule::MQTTConfigManager& mqtt() {
  return gMqttManager;
}

}  // namespace SendDataTask

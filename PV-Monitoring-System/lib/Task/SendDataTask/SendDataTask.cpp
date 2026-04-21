#include "SendDataTask.h"

#include "../../Modules/PV_Sense/PVData.h"
#include "../WifiTask/WifiTask.h"
#include "../dataTask/dataTask.h"

#include <WiFi.h>
#include <cstdio>
#include <cstring>

namespace SendDataTask {

namespace {

Config gConfig;
TaskHandle_t gTaskHandle = nullptr;
MQTTModule::MQTTConfigManager gMqttManager;
InfluxModule::InfluxClient gInfluxClient;
Status gStatus = Status::Idle;
bool gStarted = false;
bool gLastMqttConnected = false;
bool gStatusPublished = false;
unsigned long gLastPublishMs = 0;
unsigned long gLastStatusMs = 0;

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
  gInfluxClient.printStatus(Serial);
  Serial.printf("[SEND] Data Interval  : %lu ms\n",
                static_cast<unsigned long>(gConfig.publishIntervalMs));
  Serial.printf("[SEND] Status Interval: %lu ms\n",
                static_cast<unsigned long>(gConfig.statusIntervalMs));
  Serial.println();
}

size_t buildStatusPayload(char* buffer, size_t bufferSize) {
  const long rssi = static_cast<long>(WiFi.RSSI());
  const String ip = WifiTask::getIpAddress();
  const String ssid = WifiTask::getSsid();
  const unsigned long uptimeMs = millis();

  const int written = std::snprintf(
      buffer,
      bufferSize,
      "{\"device_id\":\"%s\","
      "\"firmware_version\":\"%s\","
      "\"wifi_ssid\":\"%s\","
      "\"wifi_rssi\":%ld,"
      "\"ip\":\"%s\","
      "\"uptime_ms\":%lu,"
      "\"status\":\"online\"}",
      gMqttManager.getDeviceId(),
      gConfig.firmwareVersion,
      ssid.c_str(),
      rssi,
      ip.c_str(),
      uptimeMs);

  if (written <= 0 || written >= static_cast<int>(bufferSize)) {
    return 0;
  }
  return static_cast<size_t>(written);
}

bool publishStatus() {
  char payload[384] = {};
  if (buildStatusPayload(payload, sizeof(payload)) == 0U) {
    return false;
  }
  return gMqttManager.publishStatus(payload);
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

size_t buildLineProtocol(char* buffer,
                         size_t bufferSize,
                         const PVSense::MpptData& pvData,
                         const InverterSense::InverterSnapshot& inverterData,
                         bool hasInverterData) {
  const InfluxModule::Config influxConfig = gInfluxClient.getConfig();
  const char* measurement =
      (influxConfig.measurement && influxConfig.measurement[0] != '\0')
          ? influxConfig.measurement
          : InfluxModule::kDefaultMeasurement;

  const int written = std::snprintf(
      buffer,
      bufferSize,
      "%s,device_id=%s,charging_status=%u "
      "mppt_pv_voltage=%.3f,"
      "mppt_charging_power=%.3f,"
      "mppt_charging_current=%.3f,"
      "mppt_battery_voltage=%.3f,"
      "mppt_load_current=%.3f,"
      "mppt_load_power=%.3f,"
      "mppt_fault_code=%ui,"
      "inverter_valid=%s,"
      "inverter_ac_voltage=%.3f,"
      "inverter_ac_current=%.3f,"
      "inverter_ac_power=%.3f,"
      "inverter_ac_energy=%.3f,"
      "inverter_ac_frequency=%.3f,"
      "inverter_ac_power_factor=%.3f,"
      "inverter_ac_apparent_power=%.3f,"
      "wifi_rssi=%ldi",
      measurement,
      gMqttManager.getDeviceId(),
      static_cast<unsigned>(pvData.chargingStatus),
      static_cast<double>(pvData.pvVoltage),
      static_cast<double>(pvData.chargingPower),
      static_cast<double>(pvData.chargingCurrent),
      static_cast<double>(pvData.batteryVoltage),
      static_cast<double>(pvData.loadCurrent),
      static_cast<double>(pvData.loadPower),
      static_cast<unsigned>(pvData.faultCode),
      hasInverterData ? "true" : "false",
      static_cast<double>(hasInverterData ? inverterData.voltage : 0.0f),
      static_cast<double>(hasInverterData ? inverterData.current : 0.0f),
      static_cast<double>(hasInverterData ? inverterData.power : 0.0f),
      static_cast<double>(hasInverterData ? inverterData.energy : 0.0f),
      static_cast<double>(hasInverterData ? inverterData.frequency : 0.0f),
      static_cast<double>(hasInverterData ? inverterData.powerFactor : 0.0f),
      static_cast<double>(hasInverterData ? inverterData.apparentPower : 0.0f),
      static_cast<long>(WiFi.RSSI()));

  if (written <= 0 || written >= static_cast<int>(bufferSize)) {
    return 0;
  }
  return static_cast<size_t>(written);
}

void handleIncomingMessage(const char* topic, const uint8_t* payload, unsigned int length) {
  if (topic == nullptr || std::strcmp(topic, gMqttManager.getSubsTopic()) != 0) {
    return;
  }

  char message[64] = {};
  const size_t copyLength = (length < sizeof(message) - 1U) ? length : sizeof(message) - 1U;
  std::memcpy(message, payload, copyLength);
  message[copyLength] = '\0';

  // Trim trailing whitespace/newline.
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

  // Accept either a plain command string ("ping") or a JSON like {"cmd":"ping"}.
  const bool isPing =
      (std::strcmp(message, "ping") == 0) || (std::strstr(message, "\"ping\"") != nullptr);

  if (isPing) {
    const bool ok = publishStatus();
    if (gConfig.printEventsToSerial) {
      Serial.printf("[SUBS] ping -> status publish %s\n", ok ? "OK" : "FAIL");
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

  bool influxOk = false;
  {
    char linePayload[1024] = {};
    if (buildLineProtocol(linePayload, sizeof(linePayload), pvData, inverterData,
                          hasInverterData) > 0U) {
      influxOk = gInfluxClient.writeLineProtocol(linePayload);
    }
  }

  if (gConfig.printEventsToSerial) {
    Serial.printf(
        "[SEND] MQTT=%s | INFLUX=%s (http=%d) | MPPT[pv=%.1fV batt=%.1fV] | INV[%s]\n",
        mqttOk ? "OK" : "FAIL",
        influxOk ? "OK" : (gInfluxClient.getConfig().enabled ? "FAIL" : "OFF"),
        gInfluxClient.getLastHttpCode(),
        pvData.pvVoltage,
        pvData.batteryVoltage,
        hasInverterData ? "data" : "no data");
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
      setStatus(Status::WaitingWifi);
    } else if (!gMqttManager.ensureConnected()) {
      setStatus(Status::ConnectingBroker);
    } else {
      gMqttManager.loop();

      const unsigned long now = millis();

      if (!gStatusPublished && gConfig.publishStatusOnConnect) {
        if (publishStatus()) {
          gStatusPublished = true;
          gLastStatusMs = now;
          if (gConfig.printEventsToSerial) {
            Serial.println("[SEND] Status -> MQTT: OK");
          }
        }
      } else if (gConfig.statusIntervalMs > 0U &&
                 (now - gLastStatusMs) >= gConfig.statusIntervalMs) {
        if (publishStatus()) {
          gLastStatusMs = now;
        }
      }

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
  gInfluxClient.begin(gConfig.influxConfig);

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

InfluxModule::InfluxClient& influx() {
  return gInfluxClient;
}

}  // namespace SendDataTask

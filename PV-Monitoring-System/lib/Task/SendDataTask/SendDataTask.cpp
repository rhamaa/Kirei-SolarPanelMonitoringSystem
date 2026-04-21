#include "SendDataTask.h"

#include "../../Modules/PV_Sense/PVData.h"
#include "../WifiTask/WifiTask.h"
#include "../dataTask/dataTask.h"

#include <WiFi.h>
#include <cstdio>

namespace SendDataTask {

namespace {

Config gConfig;
TaskHandle_t gTaskHandle = nullptr;
MQTTModule::MQTTConfigManager gMqttManager;
Status gStatus = Status::Idle;
bool gStarted = false;
bool gLastMqttConnected = false;
bool gInfoPublished = false;
bool gDiscoveryPublished = false;
bool gAvailabilityPublished = false;
unsigned long gLastPublishMs = 0;

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

  const MQTTModule::Config mqttConfig = gMqttManager.getConfig();

  Serial.println();
  Serial.println("=====================================");
  Serial.println("          MQTT SendDataTask");
  Serial.println("=====================================");
  Serial.printf("[SEND] Broker    : %s:%u\n", mqttConfig.server, mqttConfig.port);
  Serial.printf("[SEND] Client ID : %s\n", mqttConfig.clientId);
  Serial.printf("[SEND] Device ID : %s\n", gConfig.deviceId);
  Serial.printf("[SEND] StateTopic: %s\n", mqttConfig.dataTopic);
  Serial.printf("[SEND] AvailTopic: %s\n", mqttConfig.availabilityTopic);
  Serial.printf("[SEND] Interval  : %lu ms\n", static_cast<unsigned long>(gConfig.publishIntervalMs));
  Serial.println();
}

struct SensorDef {
  const char* sensorId;
  const char* name;
  const char* jsonKey;
  const char* unit;
  const char* deviceClass;
  const char* stateClass;
  const char* icon;
};

static const SensorDef kSensors[] = {
  {"pv_power",           "PV Power",           "pv_power",        "W",   "power",   "measurement",      "mdi:solar-power"},
  {"pv_voltage",         "PV Voltage",         "pv_voltage",      "V",   "voltage", "measurement",      "mdi:flash"},
  {"pv_current",         "PV Current",         "pv_current",      "A",   "current", "measurement",      "mdi:current-dc"},
  {"energy_today",       "Energy Today",       "energy_today",    "kWh", "energy",  "total_increasing", "mdi:solar-power"},
  {"battery_soc",        "Battery SOC",        "batt_soc",        "%",   "battery", "measurement",      "mdi:battery"},
  {"battery_voltage",    "Battery Voltage",    "batt_voltage",    "V",   "voltage", "measurement",      "mdi:battery-charging"},
  {"battery_energy_in",  "Battery Energy In",  "batt_energy_in",  "kWh", "energy",  "total_increasing", "mdi:battery-arrow-up"},
  {"battery_energy_out", "Battery Energy Out", "batt_energy_out", "kWh", "energy",  "total_increasing", "mdi:battery-arrow-down"},
  {"load1_power",        "Load 1 Power",       "load1_power",     "W",   "power",   "measurement",      "mdi:power-plug"},
  {"load1_energy",       "Load 1 Energy",      "load1_energy",    "kWh", "energy",  "total_increasing", "mdi:lightning-bolt"},
};
static constexpr size_t kSensorCount = sizeof(kSensors) / sizeof(kSensors[0]);

bool publishHaDiscovery() {
  const char* deviceId = gConfig.deviceId;
  const MQTTModule::Config mqttCfg = gMqttManager.getConfig();

  bool allOk = true;
  for (size_t i = 0; i < kSensorCount; ++i) {
    const SensorDef& s = kSensors[i];

    char topic[128] = {};
    std::snprintf(topic, sizeof(topic),
                  "homeassistant/sensor/%s/%s/config",
                  deviceId, s.sensorId);

    char payload[640] = {};
    const int written = std::snprintf(
        payload, sizeof(payload),
        "{\"name\":\"%s\","
        "\"unique_id\":\"%s_%s\","
        "\"state_topic\":\"%s\","
        "\"availability_topic\":\"%s\","
        "\"value_template\":\"{{value_json.%s}}\","
        "\"unit_of_measurement\":\"%s\","
        "\"device_class\":\"%s\","
        "\"state_class\":\"%s\","
        "\"icon\":\"%s\","
        "\"device\":{\"identifiers\":[\"%s\"],"
        "\"name\":\"%s\","
        "\"model\":\"MPPT Solar Monitor\","
        "\"manufacturer\":\"DIY\","
        "\"sw_version\":\"%s\"}}",
        s.name,
        deviceId, s.sensorId,
        mqttCfg.dataTopic,
        mqttCfg.availabilityTopic,
        s.jsonKey,
        s.unit,
        s.deviceClass,
        s.stateClass,
        s.icon,
        deviceId,
        gConfig.deviceName,
        gConfig.swVersion);

    if (written > 0 && written < static_cast<int>(sizeof(payload))) {
      const bool ok = gMqttManager.publishJson(topic, payload, true);
      allOk = allOk && ok;
    } else {
      allOk = false;
    }
  }
  return allOk;
}

void printInfoPublishResult(bool published) {
  if (!gConfig.printEventsToSerial) {
    return;
  }

  if (published) {
    Serial.println("[SEND] Info -> MQTT: OK");
    return;
  }

  Serial.printf("[SEND] Info -> MQTT: FAIL | mqtt=%s | state=%s\n",
                MQTTModule::MQTTConfigManager::statusToString(gMqttManager.getStatus()),
                MQTTModule::MQTTConfigManager::clientStateToString(gMqttManager.getClientState()));
}

bool publishInfo() {
  const MQTTModule::Config mqttConfig = gMqttManager.getConfig();

  char payload[384] = {};
  const int written = std::snprintf(payload,
                                    sizeof(payload),
                                    "{\"client_id\":\"%s\",\"broker\":\"%s\",\"port\":%u,"
                                    "\"data_topic\":\"%s\",\"info_topic\":\"%s\"}",
                                    mqttConfig.clientId,
                                    mqttConfig.server,
                                    static_cast<unsigned>(mqttConfig.port),
                                    mqttConfig.dataTopic,
                                    mqttConfig.infoTopic);

  if (written <= 0 || written >= static_cast<int>(sizeof(payload))) {
    return false;
  }

  return gMqttManager.publishInfoJson(payload);
}

bool publishHaState(const PVSense::MpptData& data) {
  char payload[384] = {};
  const int written = std::snprintf(
      payload,
      sizeof(payload),
      "{\"pv_voltage\":%.2f,"
      "\"pv_current\":%.3f,"
      "\"pv_power\":%.1f,"
      "\"energy_today\":%.4f,"
      "\"batt_soc\":%.1f,"
      "\"batt_voltage\":%.2f,"
      "\"batt_energy_in\":%.4f,"
      "\"batt_energy_out\":%.4f,"
      "\"load1_power\":%.1f,"
      "\"load1_energy\":%.4f}",
      static_cast<double>(data.pvVoltage),
      static_cast<double>(data.chargingCurrent),
      static_cast<double>(data.chargingPower),
      0.0,
      0.0,
      static_cast<double>(data.batteryVoltage),
      0.0,
      0.0,
      static_cast<double>(data.loadPower),
      0.0);

  if (written <= 0 || written >= static_cast<int>(sizeof(payload))) {
    return false;
  }

  return gMqttManager.publishDataJson(payload, true);
}

void printDataPublishResult(const PVSense::MpptData& data, bool published) {
  if (!gConfig.printEventsToSerial) {
    return;
  }

  if (published) {
    Serial.printf("[SEND] State -> HA : OK | pv=%.1fW %.2fV | batt=%.2fV | load=%.1fW\n",
                  data.chargingPower,
                  data.pvVoltage,
                  data.batteryVoltage,
                  data.loadPower);
    return;
  }

  Serial.printf("[SEND] State -> HA : FAIL | mqtt=%s | state=%s | wifi=%s\n",
                MQTTModule::MQTTConfigManager::statusToString(gMqttManager.getStatus()),
                MQTTModule::MQTTConfigManager::clientStateToString(gMqttManager.getClientState()),
                WifiTask::isConnected() ? "Connected" : "Disconnected");
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
      gInfoPublished = false;
      gDiscoveryPublished = false;
      gAvailabilityPublished = false;
      setStatus(Status::WaitingWifi);
    } else if (!gMqttManager.ensureConnected()) {
      gAvailabilityPublished = false;
      setStatus(Status::ConnectingBroker);
    } else {
      gMqttManager.loop();

      if (!gDiscoveryPublished) {
        gDiscoveryPublished = publishHaDiscovery();
        if (gConfig.printEventsToSerial) {
          Serial.printf("[SEND] HA Discovery: %s\n", gDiscoveryPublished ? "OK" : "FAIL");
        }
      }

      if (!gAvailabilityPublished) {
        gAvailabilityPublished = gMqttManager.publishAvailability(true, true);
        if (gConfig.printEventsToSerial) {
          Serial.printf("[SEND] HA Avail    : %s\n", gAvailabilityPublished ? "online" : "FAIL");
        }
      }

      if (!gInfoPublished && gConfig.publishInfoOnConnect) {
        gInfoPublished = publishInfo();
        printInfoPublishResult(gInfoPublished);
      }

      PVSense::MpptData pvData{};
      if (!DataTask::getLatestPVData(pvData)) {
        setStatus(Status::WaitingPVData);
      } else {
        setStatus(Status::Ready);

        const unsigned long now = millis();
        if ((gLastPublishMs == 0U) || ((now - gLastPublishMs) >= gConfig.publishIntervalMs)) {
          const bool published = publishHaState(pvData);
          printDataPublishResult(pvData, published);
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

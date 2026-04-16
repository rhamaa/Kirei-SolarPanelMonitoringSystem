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
  Serial.printf("[SEND] DataTopic : %s\n", mqttConfig.dataTopic);
  Serial.printf("[SEND] InfoTopic : %s\n", mqttConfig.infoTopic);
  Serial.printf("[SEND] Interval  : %lu ms\n", static_cast<unsigned long>(gConfig.publishIntervalMs));
  Serial.println();
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

bool publishDataSnapshot(const PVSense::MpptData& data) {
  InverterSense::InverterSnapshot inverterData{};
  const bool hasInverterData = DataTask::getLatestInverterData(inverterData);

  char payload[1024] = {};
  const int written = std::snprintf(
      payload,
      sizeof(payload),
      "{\"mppt_valid\":true,"
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
      static_cast<double>(data.pvVoltage),
      static_cast<double>(data.chargingPower),
      static_cast<double>(data.chargingCurrent),
      static_cast<double>(data.batteryVoltage),
      static_cast<unsigned>(data.chargingStatus),
      PVSense::PVData::chargingStatusToString(data.chargingStatus),
      static_cast<double>(data.loadCurrent),
      static_cast<double>(data.loadPower),
      static_cast<unsigned>(data.faultCode),
      static_cast<unsigned long>(data.timestampMs),
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

  if (written <= 0 || written >= static_cast<int>(sizeof(payload))) {
    return false;
  }

  return gMqttManager.publishDataJson(payload);
}

void printDataPublishResult(const PVSense::MpptData& data, bool published) {
  if (!gConfig.printEventsToSerial) {
    return;
  }

  if (published) {
    InverterSense::InverterSnapshot inverterData{};
    const bool hasInverterData = DataTask::getLatestInverterData(inverterData);

    if (hasInverterData) {
      Serial.printf(
          "[SEND] Data -> MQTT: OK | MPPT[pv=%.1fV batt=%.1fV load=%.1fW] | "
          "INV[ac=%.1fW]\n",
          data.pvVoltage,
          data.batteryVoltage,
          data.loadPower,
          inverterData.power);
    } else {
      Serial.printf("[SEND] Data -> MQTT: OK | MPPT[pv=%.1fV batt=%.1fV load=%.1fW] | "
                    "INV[no data]\n",
                    data.pvVoltage,
                    data.batteryVoltage,
                    data.loadPower);
    }
    return;
  }

  Serial.printf("[SEND] Data -> MQTT: FAIL | mqtt=%s | state=%s | wifi=%s\n",
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
      setStatus(Status::WaitingWifi);
    } else if (!gMqttManager.ensureConnected()) {
      setStatus(Status::ConnectingBroker);
    } else {
      gMqttManager.loop();

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
          const bool published = publishDataSnapshot(pvData);
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

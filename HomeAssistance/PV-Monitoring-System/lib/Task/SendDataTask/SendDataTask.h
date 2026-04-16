#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>

#include "../../Modules/MQTTConfig/MQTTConfig.h"

namespace SendDataTask {

enum class Status : uint8_t {
  Idle = 0,
  WaitingWifi,
  WaitingPVData,
  ConnectingBroker,
  Ready,
  PublishSuccess,
  PublishFailed,
};

struct Config {
  uint16_t stackSize = 6144;
  UBaseType_t priority = 1;
  BaseType_t coreId = tskNO_AFFINITY;
  uint32_t taskIntervalMs = 1000;
  uint32_t publishIntervalMs = 10000;
  bool printEventsToSerial = true;
  bool publishInfoOnConnect = true;
  MQTTModule::Config mqttConfig{};
};

bool begin(const Config& config = Config{});
bool isConnected();
Status getStatus();
MQTTModule::MQTTConfigManager& mqtt();

}  // namespace SendDataTask

#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>

#include "../../Modules/InfluxConfig/InfluxConfig.h"
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
  uint16_t stackSize = 8192;
  UBaseType_t priority = 1;
  BaseType_t coreId = tskNO_AFFINITY;
  uint32_t taskIntervalMs = 1000;
  uint32_t publishIntervalMs = 10000;
  uint32_t statusIntervalMs = 30000;
  const char* firmwareVersion = "1.0.0";
  bool printEventsToSerial = true;
  bool publishStatusOnConnect = true;
  MQTTModule::Config mqttConfig{};
  InfluxModule::Config influxConfig{};
};

bool begin(const Config& config = Config{});
bool isConnected();
Status getStatus();
MQTTModule::MQTTConfigManager& mqtt();
InfluxModule::InfluxClient& influx();

}  // namespace SendDataTask

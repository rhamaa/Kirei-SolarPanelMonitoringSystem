#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>

#include "../../Modules/WifiConfig/WifiConfig.h"

namespace WifiTask {

struct Config {
  uint16_t stackSize = 8192;
  UBaseType_t priority = 1;
  BaseType_t coreId = tskNO_AFFINITY;
  uint32_t monitorIntervalMs = 10000;
  bool printEventsToSerial = true;
  WifiModule::Config wifiConfig{};
};

bool begin(const Config& config = Config{});
bool isConnected();
String getIpAddress();
String getSsid();
WifiModule::Status getStatus();
WifiModule::WifiConfigManager& manager();

}  // namespace WifiTask

#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>

#include "../../Modules/MQTTConfig/MQTTConfig.h"

namespace SendDataTask {

/// Konfigurasi InfluxDB v2 (library tobiasschuerg/ESP8266 Influxdb).
/// `org` boleh nama organisasi atau **org ID** hex dari UI Influx.
struct InfluxSettings {
  const char* url = nullptr;
  const char* org = nullptr;
  const char* bucket = nullptr;
  const char* token = nullptr;
  const char* measurement = "pv_monitoring";
  /// POSIX TZ untuk NTP (contoh resmi Influx: "UTC7").
  const char* tzInfo = "UTC7";
  bool enabled = false;
};

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
  uint32_t publishIntervalMs = 5000;
  uint32_t statusIntervalMs = 30000;
  const char* firmwareVersion = "1.0.0";
  bool printEventsToSerial = true;
  bool publishStatusOnConnect = true;
  MQTTModule::Config mqttConfig{};
  InfluxSettings influx{};
};

bool begin(const Config& config = Config{});
bool isConnected();
Status getStatus();
MQTTModule::MQTTConfigManager& mqtt();

}  // namespace SendDataTask

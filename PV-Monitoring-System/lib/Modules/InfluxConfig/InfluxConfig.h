#pragma once

#include <Arduino.h>

namespace InfluxModule {

static constexpr char kDefaultUrl[] = "http://192.168.68.106:8086";
static constexpr char kDefaultMeasurement[] = "pv_monitoring";
static constexpr char kDefaultPrecision[] = "ms";

enum class Status : uint8_t {
  Idle = 0,
  NoWifi,
  Ok,
  HttpError,
  BadConfig,
  WriteFailed,
};

struct Config {
  const char* url = kDefaultUrl;
  const char* org = "";
  const char* bucket = "";
  const char* token = "";
  const char* measurement = kDefaultMeasurement;
  const char* precision = kDefaultPrecision;
  uint32_t timeoutMs = 3000;
  bool enabled = true;
};

class InfluxClient {
 public:
  bool begin(const Config& config = Config{});
  bool writeLineProtocol(const char* lineProtocol);

  Status getStatus() const;
  int getLastHttpCode() const;
  Config getConfig() const;

  void printStatus(Print& out) const;
  static const char* statusToString(Status status);

 private:
  Config config_{};
  Status status_ = Status::Idle;
  int lastHttpCode_ = 0;
  bool initialized_ = false;

  bool isConfigValid() const;
  bool buildWriteUrl(char* buffer, size_t bufferSize) const;
};

}  // namespace InfluxModule

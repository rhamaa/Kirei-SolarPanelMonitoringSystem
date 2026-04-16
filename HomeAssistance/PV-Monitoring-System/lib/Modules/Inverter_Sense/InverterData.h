#pragma once

#include <Arduino.h>
#include <HardwareSerial.h>
#include <PZEM004Tv30.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

namespace InverterSense {

static constexpr uint8_t kDefaultUartPort = 2;
static constexpr uint8_t kDefaultPzemRxPin = 4;
static constexpr uint8_t kDefaultPzemTxPin = 5;
static constexpr uint8_t kDefaultLedPin = 2;
static constexpr uint32_t kDefaultReadIntervalMs = 2000;
static constexpr float kDefaultMaxVoltage = 240.0f;
static constexpr float kDefaultMinVoltage = 180.0f;
static constexpr float kDefaultMaxCurrent = 10.0f;
static constexpr float kDefaultMaxPower = 2000.0f;

struct InverterSnapshot {
  float voltage = 0.0f;
  float current = 0.0f;
  float power = 0.0f;
  float energy = 0.0f;
  float frequency = 0.0f;
  float powerFactor = 0.0f;
  float apparentPower = 0.0f;
  uint32_t timestampMs = 0;
};

struct InverterConfig {
  uint8_t uartPort = kDefaultUartPort;
  uint8_t rxPin = kDefaultPzemRxPin;
  uint8_t txPin = kDefaultPzemTxPin;
  uint8_t ledPin = kDefaultLedPin;
  uint32_t readIntervalMs = kDefaultReadIntervalMs;
  float maxVoltage = kDefaultMaxVoltage;
  float minVoltage = kDefaultMinVoltage;
  float maxCurrent = kDefaultMaxCurrent;
  float maxPower = kDefaultMaxPower;
  bool blinkLedOnError = true;
  bool blinkLedOnAlert = true;
};

enum class ReadStatus : uint8_t {
  Idle = 0,
  Ok,
  NotInitialized,
  ReadFailed,
};

class InverterData {
 public:
  explicit InverterData(uint8_t uartPort = kDefaultUartPort);

  bool begin(const InverterConfig& config = InverterConfig{});
  bool poll();
  bool resetEnergy();

  bool getLatestData(InverterSnapshot& out) const;
  bool hasValidData() const;

  InverterConfig getConfig() const;
  ReadStatus getLastReadStatus() const;

  void printLastData(Print& out) const;
  static const char* readStatusToString(ReadStatus status);
  static void printData(Print& out, const InverterSnapshot& data);
  static float calculateApparentPower(float power, float powerFactor);

 private:
  static bool isFiniteValue(float value);
  static float sanitizeReading(float value);

  bool hasAlert(const InverterSnapshot& data) const;
  void printAlerts(Print& out, const InverterSnapshot& data) const;
  void blinkLed(uint8_t times, uint32_t onMs, uint32_t offMs) const;

  HardwareSerial serial_;
  PZEM004Tv30* pzem_ = nullptr;
  InverterConfig config_{};
  mutable SemaphoreHandle_t mutex_ = nullptr;
  InverterSnapshot latestData_{};
  bool hasValidData_ = false;
  bool initialized_ = false;
  ReadStatus lastReadStatus_ = ReadStatus::NotInitialized;
};

}  // namespace InverterSense

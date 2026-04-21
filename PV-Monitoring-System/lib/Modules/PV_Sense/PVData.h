#pragma once

#include <Arduino.h>
#include <HardwareSerial.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

namespace PVSense {

static constexpr uint8_t kDefaultRs485Tx = 16;
static constexpr uint8_t kDefaultRs485Rx = 17;
static constexpr uint8_t kDefaultSlaveId = 0x06;
static constexpr uint32_t kDefaultPollMs = 2000;
static constexpr uint32_t kDefaultBaudRate = 9600;
static constexpr uint32_t kDefaultResponseTimeoutMs = 500;

struct MpptData {
  float pvVoltage = 0.0f;
  float chargingPower = 0.0f;
  float chargingCurrent = 0.0f;
  float batteryVoltage = 0.0f;
  uint8_t chargingStatus = 0;
  float loadCurrent = 0.0f;
  float loadPower = 0.0f;
  uint16_t faultCode = 0;
  uint32_t timestampMs = 0;
};

struct PVConfig {
  uint8_t uartPort = 1;
  uint32_t baudRate = kDefaultBaudRate;
  uint8_t txPin = kDefaultRs485Tx;
  uint8_t rxPin = kDefaultRs485Rx;
  uint8_t slaveId = kDefaultSlaveId;
  uint32_t pollIntervalMs = kDefaultPollMs;
  uint32_t responseTimeoutMs = kDefaultResponseTimeoutMs;
};

enum class PollStatus : uint8_t {
  Idle = 0,
  Ok,
  NotInitialized,
  Timeout,
  ShortResponse,
  UnexpectedLength,
  InvalidSlaveId,
  InvalidFunction,
  InvalidPayloadLength,
  InvalidCrc,
};

class PVData {
 public:
  explicit PVData(uint8_t uartPort = 1);

  bool begin(const PVConfig& config = PVConfig{});
  bool poll();

  bool getLatestData(MpptData& out) const;
  bool hasValidData() const;

  PVConfig getConfig() const;
  PollStatus getLastPollStatus() const;
  size_t getLastResponseLength() const;

  void printRequestFrame(Print& out) const;
  void printLastResponse(Print& out) const;
  void printLastData(Print& out) const;

  static const char* chargingStatusToString(uint8_t status);
  static const char* pollStatusToString(PollStatus status);
  static void printData(Print& out, const MpptData& data);
  static void printFaults(Print& out, uint16_t faultCode);

 private:
  static constexpr uint8_t kFunctionCode = 0x12;
  static constexpr uint16_t kStartRegister = 0x1000;
  static constexpr uint8_t kRegisterCount = 0x24;
  static constexpr size_t kRequestFrameLength = 8;
  static constexpr size_t kExpectedDataBytes = kRegisterCount * 2;
  static constexpr size_t kExpectedPayloadBytes = 4 + kExpectedDataBytes;
  static constexpr size_t kExpectedResponseLength = 1 + 1 + 1 + kExpectedPayloadBytes + 2;
  static constexpr size_t kResponseBufferSize = 100;

  HardwareSerial serial_;
  PVConfig config_{};
  mutable SemaphoreHandle_t mutex_ = nullptr;
  MpptData latestData_{};
  bool hasValidData_ = false;
  bool initialized_ = false;
  PollStatus lastPollStatus_ = PollStatus::NotInitialized;
  uint8_t requestFrame_[kRequestFrameLength] = {};
  uint8_t responseBuffer_[kResponseBufferSize] = {};
  size_t responseLength_ = 0;

  static uint16_t crc16(const uint8_t* data, size_t length);
  static bool crcOk(const uint8_t* buffer, size_t length);
  static void appendCrc(uint8_t* buffer, size_t length);
  static uint16_t getRegister(const uint8_t* data, uint8_t index);

  void buildRequestFrame();
  size_t readResponse(uint8_t* buffer, size_t bufferSize);
  PollStatus parseResponse(const uint8_t* buffer, size_t length, MpptData& data) const;
};

}  // namespace PVSense

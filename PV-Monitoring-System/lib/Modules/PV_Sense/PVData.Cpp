#include "PVData.h"

#include <cstring>

namespace PVSense {

PVData::PVData(uint8_t uartPort) : serial_(uartPort) {
  config_.uartPort = uartPort;
}

bool PVData::begin(const PVConfig& config) {
  config_ = config;

  if (mutex_ == nullptr) {
    mutex_ = xSemaphoreCreateMutex();
    if (mutex_ == nullptr) {
      return false;
    }
  }

  serial_.begin(config_.baudRate, SERIAL_8N1, config_.rxPin, config_.txPin);
  buildRequestFrame();

  if (xSemaphoreTake(mutex_, portMAX_DELAY) == pdTRUE) {
    responseLength_ = 0;
    hasValidData_ = false;
    lastPollStatus_ = PollStatus::Idle;
    std::memset(responseBuffer_, 0, sizeof(responseBuffer_));
    xSemaphoreGive(mutex_);
  }

  initialized_ = true;
  return true;
}

bool PVData::poll() {
  if (!initialized_) {
    if (mutex_ != nullptr && xSemaphoreTake(mutex_, portMAX_DELAY) == pdTRUE) {
      lastPollStatus_ = PollStatus::NotInitialized;
      responseLength_ = 0;
      xSemaphoreGive(mutex_);
    }
    return false;
  }

  uint8_t localResponse[kResponseBufferSize] = {};
  const size_t localLength = readResponse(localResponse, sizeof(localResponse));

  MpptData parsedData{};
  const PollStatus status = parseResponse(localResponse, localLength, parsedData);

  if (mutex_ != nullptr && xSemaphoreTake(mutex_, portMAX_DELAY) == pdTRUE) {
    responseLength_ = localLength;
    std::memcpy(responseBuffer_, localResponse, sizeof(responseBuffer_));
    lastPollStatus_ = status;

    if (status == PollStatus::Ok) {
      latestData_ = parsedData;
      hasValidData_ = true;
    }

    xSemaphoreGive(mutex_);
  }

  return status == PollStatus::Ok;
}

bool PVData::getLatestData(MpptData& out) const {
  if (mutex_ == nullptr) {
    return false;
  }

  if (xSemaphoreTake(mutex_, portMAX_DELAY) != pdTRUE) {
    return false;
  }

  const bool available = hasValidData_;
  if (available) {
    out = latestData_;
  }

  xSemaphoreGive(mutex_);
  return available;
}

bool PVData::hasValidData() const {
  if (mutex_ == nullptr) {
    return false;
  }

  if (xSemaphoreTake(mutex_, portMAX_DELAY) != pdTRUE) {
    return false;
  }

  const bool available = hasValidData_;
  xSemaphoreGive(mutex_);
  return available;
}

PVConfig PVData::getConfig() const {
  return config_;
}

PollStatus PVData::getLastPollStatus() const {
  if (mutex_ == nullptr) {
    return PollStatus::NotInitialized;
  }

  if (xSemaphoreTake(mutex_, portMAX_DELAY) != pdTRUE) {
    return PollStatus::NotInitialized;
  }

  const PollStatus status = lastPollStatus_;
  xSemaphoreGive(mutex_);
  return status;
}

size_t PVData::getLastResponseLength() const {
  if (mutex_ == nullptr) {
    return 0;
  }

  if (xSemaphoreTake(mutex_, portMAX_DELAY) != pdTRUE) {
    return 0;
  }

  const size_t length = responseLength_;
  xSemaphoreGive(mutex_);
  return length;
}

void PVData::printRequestFrame(Print& out) const {
  for (size_t i = 0; i < kRequestFrameLength; ++i) {
    out.printf("%02X ", static_cast<unsigned>(requestFrame_[i]));
  }
  out.println();
}

void PVData::printLastResponse(Print& out) const {
  if (mutex_ == nullptr) {
    out.println("[empty]");
    return;
  }

  uint8_t localCopy[kResponseBufferSize] = {};
  size_t length = 0;

  if (xSemaphoreTake(mutex_, portMAX_DELAY) == pdTRUE) {
    length = responseLength_;
    std::memcpy(localCopy, responseBuffer_, sizeof(localCopy));
    xSemaphoreGive(mutex_);
  }

  if (length == 0) {
    out.println("[empty]");
    return;
  }

  for (size_t i = 0; i < length; ++i) {
    out.printf("%02X ", static_cast<unsigned>(localCopy[i]));
  }
  out.println();
}

void PVData::printLastData(Print& out) const {
  MpptData snapshot{};
  if (!getLatestData(snapshot)) {
    out.println("[PV] Belum ada data valid.");
    return;
  }

  printData(out, snapshot);
}

const char* PVData::chargingStatusToString(uint8_t status) {
  switch (status) {
    case 0:
      return "MPPT Charging";
    case 1:
      return "Boost / Equalizing";
    case 2:
      return "Floating";
    case 3:
      return "Not Charging";
    default:
      return "Unknown";
  }
}

const char* PVData::pollStatusToString(PollStatus status) {
  switch (status) {
    case PollStatus::Idle:
      return "Idle";
    case PollStatus::Ok:
      return "OK";
    case PollStatus::NotInitialized:
      return "Not Initialized";
    case PollStatus::Timeout:
      return "Timeout";
    case PollStatus::ShortResponse:
      return "Short Response";
    case PollStatus::UnexpectedLength:
      return "Unexpected Length";
    case PollStatus::InvalidSlaveId:
      return "Invalid Slave ID";
    case PollStatus::InvalidFunction:
      return "Invalid Function Code";
    case PollStatus::InvalidPayloadLength:
      return "Invalid Payload Length";
    case PollStatus::InvalidCrc:
      return "Invalid CRC";
    default:
      return "Unknown";
  }
}

void PVData::printData(Print& out, const MpptData& data) {
  out.println();
  out.println("+==============================================+");
  out.println("|         JN-MPPT-AL1.8 - Data Terbaru         |");
  out.println("+==============================================+");
  out.println("|  [ PV / Solar ]                              |");
  out.printf("  PV Voltage    : %.1f V\n", data.pvVoltage);
  out.printf("  Charging Pwr  : %.1f W\n", data.chargingPower);
  out.printf("  Charging Curr : %.2f A\n", data.chargingCurrent);
  out.printf("  Status        : %s\n", chargingStatusToString(data.chargingStatus));
  out.println("|  [ Battery ]                                 |");
  out.printf("  Voltage       : %.1f V\n", data.batteryVoltage);
  out.println("|  [ Load / Beban ]                            |");
  out.printf("  Load Current  : %.2f A\n", data.loadCurrent);
  out.printf("  Load Power    : %.1f W\n", data.loadPower);
  out.println("|  [ Fault ]                                   |");
  printFaults(out, data.faultCode);
  out.println("+----------------------------------------------+");
}

void PVData::printFaults(Print& out, uint16_t faultCode) {
  if (faultCode == 0) {
    out.println("  Fault        : OK - tidak ada fault");
    return;
  }

  static const char* kFaultNames[] = {
      "Battery Over Voltage",
      "Battery Overheating",
      "Charge Over-Current",
      "Short Circuit",
      "Anti-Reverse Failed",
      "Controller Overheated",
      "Battery Not Connected",
      "Output Overload",
      "Memory Error",
      "Battery Under Voltage",
      "PV Over Voltage",
      "Battery Temp Sensor Fail",
      "Controller Temp Sensor Fail",
  };

  for (uint8_t i = 0; i < (sizeof(kFaultNames) / sizeof(kFaultNames[0])); ++i) {
    if ((faultCode & (1U << i)) != 0U) {
      out.printf("  [FAULT]      : %s\n", kFaultNames[i]);
    }
  }
}

uint16_t PVData::crc16(const uint8_t* data, size_t length) {
  uint16_t crc = 0xFFFF;

  for (size_t i = 0; i < length; ++i) {
    crc ^= data[i];
    for (uint8_t j = 0; j < 8; ++j) {
      crc = (crc & 1U) ? (crc >> 1) ^ 0xA001U : (crc >> 1);
    }
  }

  return crc;
}

bool PVData::crcOk(const uint8_t* buffer, size_t length) {
  if (length < 2U) {
    return false;
  }

  const uint16_t calculated = crc16(buffer, length - 2U);
  const uint16_t received = static_cast<uint16_t>(buffer[length - 2U]) |
                            (static_cast<uint16_t>(buffer[length - 1U]) << 8);
  return calculated == received;
}

void PVData::appendCrc(uint8_t* buffer, size_t length) {
  const uint16_t crc = crc16(buffer, length);
  buffer[length] = static_cast<uint8_t>(crc & 0xFFU);
  buffer[length + 1U] = static_cast<uint8_t>((crc >> 8) & 0xFFU);
}

uint16_t PVData::getRegister(const uint8_t* data, uint8_t index) {
  return (static_cast<uint16_t>(data[index * 2U]) << 8) |
         static_cast<uint16_t>(data[index * 2U + 1U]);
}

void PVData::buildRequestFrame() {
  requestFrame_[0] = config_.slaveId;
  requestFrame_[1] = kFunctionCode;
  requestFrame_[2] = static_cast<uint8_t>((kStartRegister >> 8) & 0xFFU);
  requestFrame_[3] = static_cast<uint8_t>(kStartRegister & 0xFFU);
  requestFrame_[4] = 0x00;
  requestFrame_[5] = kRegisterCount;
  appendCrc(requestFrame_, 6);
}

size_t PVData::readResponse(uint8_t* buffer, size_t bufferSize) {
  while (serial_.available()) {
    serial_.read();
  }

  serial_.write(requestFrame_, kRequestFrameLength);
  serial_.flush();

  const unsigned long startedAt = millis();
  size_t length = 0;

  while ((millis() - startedAt) < config_.responseTimeoutMs) {
    while (serial_.available() && length < bufferSize) {
      buffer[length++] = static_cast<uint8_t>(serial_.read());
    }

    if (length >= kExpectedResponseLength) {
      break;
    }

    delay(5);
  }

  return length;
}

PollStatus PVData::parseResponse(const uint8_t* buffer, size_t length, MpptData& data) const {
  if (length == 0U) {
    return PollStatus::Timeout;
  }

  if (length < kExpectedResponseLength) {
    return PollStatus::ShortResponse;
  }

  if (length != kExpectedResponseLength) {
    return PollStatus::UnexpectedLength;
  }

  if (buffer[0] != config_.slaveId) {
    return PollStatus::InvalidSlaveId;
  }

  if (buffer[1] != kFunctionCode) {
    return PollStatus::InvalidFunction;
  }

  if (buffer[2] != kExpectedPayloadBytes) {
    return PollStatus::InvalidPayloadLength;
  }

  if (!crcOk(buffer, length)) {
    return PollStatus::InvalidCrc;
  }

  const uint8_t* registers = buffer + 7;

  data.pvVoltage = getRegister(registers, 1) * 0.1f;
  data.chargingPower = getRegister(registers, 7) * 0.1f;
  data.chargingCurrent = getRegister(registers, 5) * 0.01f;
  data.batteryVoltage = getRegister(registers, 2) * 0.1f;
  data.chargingStatus = static_cast<uint8_t>(getRegister(registers, 12) & 0xFFU);
  data.loadCurrent = getRegister(registers, 3) * 0.01f;
  data.faultCode = getRegister(registers, 28);
  data.loadPower = data.loadCurrent * data.batteryVoltage;
  data.timestampMs = millis();

  return PollStatus::Ok;
}

}  // namespace PVSense

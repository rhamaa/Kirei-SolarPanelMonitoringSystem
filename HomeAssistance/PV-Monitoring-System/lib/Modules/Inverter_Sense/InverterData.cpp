#include "InverterData.h"

#include <math.h>

namespace InverterSense {

InverterData::InverterData(uint8_t uartPort) : serial_(uartPort) {
  config_.uartPort = uartPort;
}

bool InverterData::begin(const InverterConfig& config) {
  config_ = config;

  if (mutex_ == nullptr) {
    mutex_ = xSemaphoreCreateMutex();
    if (mutex_ == nullptr) {
      return false;
    }
  }

  if (pzem_ == nullptr) {
    pzem_ = new PZEM004Tv30(serial_, config_.rxPin, config_.txPin);
    if (pzem_ == nullptr) {
      return false;
    }
  }

  pinMode(config_.ledPin, OUTPUT);
  digitalWrite(config_.ledPin, LOW);

  if (xSemaphoreTake(mutex_, portMAX_DELAY) == pdTRUE) {
    latestData_ = InverterSnapshot{};
    hasValidData_ = false;
    lastReadStatus_ = ReadStatus::Idle;
    xSemaphoreGive(mutex_);
  }

  initialized_ = true;
  return true;
}

bool InverterData::poll() {
  if (mutex_ == nullptr) {
    return false;
  }

  bool blinkError = false;
  bool blinkAlert = false;
  bool readOk = false;

  if (xSemaphoreTake(mutex_, portMAX_DELAY) != pdTRUE) {
    return false;
  }

  if (!initialized_ || pzem_ == nullptr) {
    lastReadStatus_ = ReadStatus::NotInitialized;
    xSemaphoreGive(mutex_);
    return false;
  }

  InverterSnapshot sample{};
  sample.voltage = pzem_->voltage();
  if (!isFiniteValue(sample.voltage)) {
    lastReadStatus_ = ReadStatus::ReadFailed;
    xSemaphoreGive(mutex_);
    blinkError = config_.blinkLedOnError;
    if (blinkError) {
      blinkLed(1, 100, 100);
    }
    return false;
  }

  sample.current = sanitizeReading(pzem_->current());
  sample.power = sanitizeReading(pzem_->power());
  sample.energy = sanitizeReading(pzem_->energy());
  sample.frequency = sanitizeReading(pzem_->frequency());
  sample.powerFactor = sanitizeReading(pzem_->pf());
  sample.apparentPower = calculateApparentPower(sample.power, sample.powerFactor);
  sample.timestampMs = millis();

  latestData_ = sample;
  hasValidData_ = true;
  lastReadStatus_ = ReadStatus::Ok;
  blinkAlert = hasAlert(sample) && config_.blinkLedOnAlert;
  readOk = true;
  xSemaphoreGive(mutex_);

  if (blinkAlert) {
    blinkLed(3, 100, 100);
  }

  return readOk;
}

bool InverterData::resetEnergy() {
  if (mutex_ == nullptr) {
    return false;
  }

  if (xSemaphoreTake(mutex_, portMAX_DELAY) != pdTRUE) {
    return false;
  }

  const bool canReset = initialized_ && (pzem_ != nullptr);
  bool resetOk = false;
  if (canReset) {
    resetOk = pzem_->resetEnergy();
    if (resetOk && hasValidData_) {
      latestData_.energy = 0.0f;
      latestData_.timestampMs = millis();
    }
  } else {
    lastReadStatus_ = ReadStatus::NotInitialized;
  }

  xSemaphoreGive(mutex_);
  return resetOk;
}

bool InverterData::getLatestData(InverterSnapshot& out) const {
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

bool InverterData::hasValidData() const {
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

InverterConfig InverterData::getConfig() const {
  return config_;
}

ReadStatus InverterData::getLastReadStatus() const {
  if (mutex_ == nullptr) {
    return ReadStatus::NotInitialized;
  }

  if (xSemaphoreTake(mutex_, portMAX_DELAY) != pdTRUE) {
    return ReadStatus::NotInitialized;
  }

  const ReadStatus status = lastReadStatus_;
  xSemaphoreGive(mutex_);
  return status;
}

void InverterData::printLastData(Print& out) const {
  InverterSnapshot snapshot{};
  if (!getLatestData(snapshot)) {
    out.println("[INV] Belum ada data valid.");
    return;
  }

  printData(out, snapshot);
  printAlerts(out, snapshot);
}

const char* InverterData::readStatusToString(ReadStatus status) {
  switch (status) {
    case ReadStatus::Idle:
      return "Idle";
    case ReadStatus::Ok:
      return "OK";
    case ReadStatus::NotInitialized:
      return "Not Initialized";
    case ReadStatus::ReadFailed:
      return "Read Failed";
    default:
      return "Unknown";
  }
}

void InverterData::printData(Print& out, const InverterSnapshot& data) {
  out.println("----------------------------------------");
  out.printf("  Tegangan    : %7.1f V\n", static_cast<double>(data.voltage));
  out.printf("  Arus        : %7.3f A\n", static_cast<double>(data.current));
  out.printf("  Daya Aktif  : %7.1f W\n", static_cast<double>(data.power));
  out.printf("  Energi      : %7.3f kWh\n", static_cast<double>(data.energy));
  out.printf("  Frekuensi   : %7.1f Hz\n", static_cast<double>(data.frequency));
  out.printf("  Power Factor: %7.2f\n", static_cast<double>(data.powerFactor));
  out.printf("  Daya Semu   : %7.1f VA\n", static_cast<double>(data.apparentPower));
  out.println("----------------------------------------");
}

float InverterData::calculateApparentPower(float power, float powerFactor) {
  if (!isFiniteValue(power) || !isFiniteValue(powerFactor) || powerFactor <= 0.0f) {
    return 0.0f;
  }

  return power / powerFactor;
}

bool InverterData::isFiniteValue(float value) {
  return !isnan(value) && !isinf(value);
}

float InverterData::sanitizeReading(float value) {
  return isFiniteValue(value) ? value : 0.0f;
}

bool InverterData::hasAlert(const InverterSnapshot& data) const {
  return (data.voltage > config_.maxVoltage) ||
         ((data.voltage > 0.0f) && (data.voltage < config_.minVoltage)) ||
         (data.current > config_.maxCurrent) || (data.power > config_.maxPower);
}

void InverterData::printAlerts(Print& out, const InverterSnapshot& data) const {
  if (data.voltage > config_.maxVoltage) {
    out.printf("[ALERT] Tegangan terlalu TINGGI: %.1f V (maks %.1f V)\n",
               static_cast<double>(data.voltage),
               static_cast<double>(config_.maxVoltage));
  }

  if ((data.voltage > 0.0f) && (data.voltage < config_.minVoltage)) {
    out.printf("[ALERT] Tegangan terlalu RENDAH: %.1f V (min %.1f V)\n",
               static_cast<double>(data.voltage),
               static_cast<double>(config_.minVoltage));
  }

  if (data.current > config_.maxCurrent) {
    out.printf("[ALERT] Arus terlalu BESAR: %.3f A (maks %.1f A)\n",
               static_cast<double>(data.current),
               static_cast<double>(config_.maxCurrent));
  }

  if (data.power > config_.maxPower) {
    out.printf("[ALERT] Daya terlalu BESAR: %.1f W (maks %.1f W)\n",
               static_cast<double>(data.power),
               static_cast<double>(config_.maxPower));
  }
}

void InverterData::blinkLed(uint8_t times, uint32_t onMs, uint32_t offMs) const {
  for (uint8_t i = 0; i < times; ++i) {
    digitalWrite(config_.ledPin, HIGH);
    delay(onMs);
    digitalWrite(config_.ledPin, LOW);
    delay(offMs);
  }
}

}  // namespace InverterSense

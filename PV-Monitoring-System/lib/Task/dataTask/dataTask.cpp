#include "dataTask.h"

namespace DataTask {

namespace {

Config gConfig;
TaskHandle_t gTaskHandle = nullptr;
PVSense::PVData gPVSensor;
InverterSense::InverterData gInverterSensor;
bool gStarted = false;

void printStartupBanner() {
  if (!gConfig.printStartupBanner) {
    return;
  }

  const PVSense::PVConfig pvConfig = gPVSensor.getConfig();

  Serial.println();
  Serial.println("=====================================");
  Serial.println("  JNGE JN-MPPT-AL1.8 Serial Reader");
  Serial.println("=====================================");
  Serial.printf("TX=GPIO%u  RX=GPIO%u  Slave=0x%02X\n",
                static_cast<unsigned>(pvConfig.txPin),
                static_cast<unsigned>(pvConfig.rxPin),
                static_cast<unsigned>(pvConfig.slaveId));

  if (gConfig.printRequestFrame) {
    Serial.print("Request frame: ");
    gPVSensor.printRequestFrame(Serial);
  }

  const InverterSense::InverterConfig inverterConfig = gInverterSensor.getConfig();
  Serial.println();
  Serial.println("=====================================");
  Serial.println("      PZEM-004T Inverter Monitor");
  Serial.println("=====================================");
  Serial.printf("TX=GPIO%u  RX=GPIO%u  LED=GPIO%u\n",
                static_cast<unsigned>(inverterConfig.txPin),
                static_cast<unsigned>(inverterConfig.rxPin),
                static_cast<unsigned>(inverterConfig.ledPin));
  Serial.printf("Alert: V %.1f-%.1f | I max %.1fA | P max %.1fW\n",
                static_cast<double>(inverterConfig.minVoltage),
                static_cast<double>(inverterConfig.maxVoltage),
                static_cast<double>(inverterConfig.maxCurrent),
                static_cast<double>(inverterConfig.maxPower));
  Serial.println();
}

void dataTaskLoop(void* parameter) {
  (void)parameter;

  const uint32_t pvIntervalMs = (gConfig.pvConfig.pollIntervalMs > 0U) ? gConfig.pvConfig.pollIntervalMs
                                                                        : 1U;
  const uint32_t inverterIntervalMs =
      (gConfig.inverterConfig.readIntervalMs > 0U) ? gConfig.inverterConfig.readIntervalMs : 1U;
  const uint32_t taskIntervalMs = (pvIntervalMs < inverterIntervalMs) ? pvIntervalMs : inverterIntervalMs;
  const TickType_t intervalTicks = pdMS_TO_TICKS((taskIntervalMs > 0U) ? taskIntervalMs : 1U);
  TickType_t lastWakeTime = xTaskGetTickCount();
  uint32_t lastPvPollMs = 0;
  uint32_t lastInverterPollMs = 0;

  for (;;) {
    const uint32_t now = millis();

    if ((lastPvPollMs == 0U) || ((now - lastPvPollMs) >= pvIntervalMs)) {
      lastPvPollMs = now;

      if (gPVSensor.poll()) {
        if (gConfig.printDataToSerial) {
          gPVSensor.printLastData(Serial);
        }
      } else {
        Serial.printf("[PV] Poll gagal: %s (len=%u)\n",
                      PVSense::PVData::pollStatusToString(gPVSensor.getLastPollStatus()),
                      static_cast<unsigned>(gPVSensor.getLastResponseLength()));

        if (gPVSensor.getLastResponseLength() > 0U) {
          Serial.print("[PV] RX: ");
          gPVSensor.printLastResponse(Serial);
        }
      }
    }

    if ((lastInverterPollMs == 0U) || ((now - lastInverterPollMs) >= inverterIntervalMs)) {
      lastInverterPollMs = now;

      if (gInverterSensor.poll()) {
        if (gConfig.printInverterDataToSerial) {
          gInverterSensor.printLastData(Serial);
        }
      } else {
        Serial.printf("[INV] Baca gagal: %s\n",
                      InverterSense::InverterData::readStatusToString(
                          gInverterSensor.getLastReadStatus()));
      }
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

  if (!gPVSensor.begin(gConfig.pvConfig)) {
    return false;
  }

  if (!gInverterSensor.begin(gConfig.inverterConfig)) {
    return false;
  }

  printStartupBanner();

  BaseType_t result = pdFAIL;
  if (gConfig.coreId == tskNO_AFFINITY) {
    result = xTaskCreate(dataTaskLoop,
                         "PVDataTask",
                         gConfig.stackSize,
                         nullptr,
                         gConfig.priority,
                         &gTaskHandle);
  } else {
    result = xTaskCreatePinnedToCore(dataTaskLoop,
                                     "PVDataTask",
                                     gConfig.stackSize,
                                     nullptr,
                                     gConfig.priority,
                                     &gTaskHandle,
                                     gConfig.coreId);
  }

  gStarted = (result == pdPASS);
  return gStarted;
}

bool getLatestPVData(PVSense::MpptData& out) {
  return gPVSensor.getLatestData(out);
}

bool getLatestInverterData(InverterSense::InverterSnapshot& out) {
  return gInverterSensor.getLatestData(out);
}

PVSense::PollStatus getLastPVStatus() {
  return gPVSensor.getLastPollStatus();
}

InverterSense::ReadStatus getLastInverterStatus() {
  return gInverterSensor.getLastReadStatus();
}

bool resetInverterEnergy() {
  return gInverterSensor.resetEnergy();
}

PVSense::PVData& pvSensor() {
  return gPVSensor;
}

InverterSense::InverterData& inverterSensor() {
  return gInverterSensor;
}

}  // namespace DataTask

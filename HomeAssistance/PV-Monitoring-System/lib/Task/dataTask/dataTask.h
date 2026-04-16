#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>

#include "../../Modules/Inverter_Sense/InverterData.h"
#include "../../Modules/PV_Sense/PVData.h"

namespace DataTask {

struct Config {
  uint16_t stackSize = 4096;
  UBaseType_t priority = 1;
  BaseType_t coreId = tskNO_AFFINITY;
  bool printStartupBanner = true;
  bool printRequestFrame = true;
  bool printDataToSerial = true;
  bool printInverterDataToSerial = true;
  PVSense::PVConfig pvConfig{};
  InverterSense::InverterConfig inverterConfig{};
};

bool begin(const Config& config = Config{});
bool getLatestPVData(PVSense::MpptData& out);
bool getLatestInverterData(InverterSense::InverterSnapshot& out);
PVSense::PollStatus getLastPVStatus();
InverterSense::ReadStatus getLastInverterStatus();
bool resetInverterEnergy();
PVSense::PVData& pvSensor();
InverterSense::InverterData& inverterSensor();

}  // namespace DataTask

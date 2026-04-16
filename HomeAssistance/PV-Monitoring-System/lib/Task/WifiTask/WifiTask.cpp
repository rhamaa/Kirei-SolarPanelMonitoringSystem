#include "WifiTask.h"

namespace WifiTask {

namespace {

Config gConfig;
TaskHandle_t gTaskHandle = nullptr;
WifiModule::WifiConfigManager gWifiManager;
bool gStarted = false;
bool gLastConnected = false;
WifiModule::Status gLastStatus = WifiModule::Status::Idle;

void printStartupBanner() {
  if (!gConfig.printEventsToSerial) {
    return;
  }

  Serial.println();
  Serial.println("=====================================");
  Serial.println("         WiFi Manager Ready");
  Serial.println("=====================================");
  Serial.printf("[WIFI] Portal AP : %s\n", gWifiManager.getPortalName().c_str());
  Serial.println("[WIFI] Akan mencoba SSID tersimpan. Jika gagal, portal konfigurasi dibuka.");
  Serial.println();
}

void printStateChange(bool connected) {
  if (!gConfig.printEventsToSerial) {
    return;
  }

  Serial.printf("[WIFI] Status : %s\n",
                WifiModule::WifiConfigManager::statusToString(gWifiManager.getStatus()));

  if (connected) {
    Serial.printf("[WIFI] SSID   : %s\n", gWifiManager.getSsid().c_str());
    Serial.printf("[WIFI] IP     : %s\n", gWifiManager.getIpAddress().c_str());
  } else {
    Serial.printf("[WIFI] Portal : %s\n", gWifiManager.getPortalName().c_str());
  }
}

void wifiTaskLoop(void* parameter) {
  (void)parameter;

  const TickType_t intervalTicks =
      pdMS_TO_TICKS((gConfig.monitorIntervalMs > 0U) ? gConfig.monitorIntervalMs : 1U);
  TickType_t lastWakeTime = xTaskGetTickCount();

  for (;;) {
    const bool connected = gWifiManager.ensureConnected();
    const WifiModule::Status status = gWifiManager.getStatus();

    if ((connected != gLastConnected) || (status != gLastStatus)) {
      printStateChange(connected);
      gLastConnected = connected;
      gLastStatus = status;
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

  if (!gWifiManager.begin(gConfig.wifiConfig)) {
    return false;
  }

  printStartupBanner();

  BaseType_t result = pdFAIL;
  if (gConfig.coreId == tskNO_AFFINITY) {
    result = xTaskCreate(wifiTaskLoop,
                         "WifiTask",
                         gConfig.stackSize,
                         nullptr,
                         gConfig.priority,
                         &gTaskHandle);
  } else {
    result = xTaskCreatePinnedToCore(wifiTaskLoop,
                                     "WifiTask",
                                     gConfig.stackSize,
                                     nullptr,
                                     gConfig.priority,
                                     &gTaskHandle,
                                     gConfig.coreId);
  }

  gStarted = (result == pdPASS);
  return gStarted;
}

bool isConnected() {
  return gWifiManager.isConnected();
}

String getIpAddress() {
  return gWifiManager.getIpAddress();
}

String getSsid() {
  return gWifiManager.getSsid();
}

WifiModule::Status getStatus() {
  return gWifiManager.getStatus();
}

WifiModule::WifiConfigManager& manager() {
  return gWifiManager;
}

}  // namespace WifiTask

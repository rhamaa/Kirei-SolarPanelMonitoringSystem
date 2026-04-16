#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>

namespace WifiModule {

enum class Status : uint8_t {
  Idle = 0,
  Connecting,
  PortalActive,
  Connected,
  Disconnected,
};

struct Config {
  const char* portalNamePrefix = "PVMonitor";
  const char* portalPassword = nullptr;
  uint32_t reconnectTimeoutMs = 15000;
  uint16_t connectTimeoutSeconds = 20;
  uint16_t portalTimeoutSeconds = 180;
  bool resetStoredCredentials = false;
  bool enableDebugOutput = true;
};

class WifiConfigManager {
 public:
  bool begin(const Config& config = Config{});
  bool ensureConnected();
  bool isConnected() const;

  Status getStatus() const;
  String getSsid() const;
  String getIpAddress() const;
  String getPortalName() const;

  void printStatus(Print& out) const;
  static const char* statusToString(Status status);

 private:
  static constexpr size_t kPortalNameLength = 32;

  Config config_{};
  WiFiManager manager_{};
  Status status_ = Status::Idle;
  bool initialized_ = false;
  char portalName_[kPortalNameLength + 1] = {};

  void buildPortalName();
  void applySettings();
  void setStatus(Status status);
  bool waitForReconnect();
  bool startConfigPortal();
};

}  // namespace WifiModule

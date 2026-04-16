#include "WifiConfig.h"

namespace WifiModule {

bool WifiConfigManager::begin(const Config& config) {
  config_ = config;

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);

  buildPortalName();
  applySettings();

  if (config_.resetStoredCredentials) {
    manager_.resetSettings();
    WiFi.disconnect(true, true);
  }

  status_ = (WiFi.status() == WL_CONNECTED) ? Status::Connected : Status::Disconnected;
  initialized_ = true;
  return true;
}

bool WifiConfigManager::ensureConnected() {
  if (!initialized_ && !begin()) {
    return false;
  }

  if (WiFi.status() == WL_CONNECTED) {
    setStatus(Status::Connected);
    return true;
  }

  if (waitForReconnect()) {
    setStatus(Status::Connected);
    return true;
  }

  return startConfigPortal();
}

bool WifiConfigManager::isConnected() const {
  return WiFi.status() == WL_CONNECTED;
}

Status WifiConfigManager::getStatus() const {
  return status_;
}

String WifiConfigManager::getSsid() const {
  return WiFi.SSID();
}

String WifiConfigManager::getIpAddress() const {
  if (WiFi.status() != WL_CONNECTED) {
    return String();
  }

  return WiFi.localIP().toString();
}

String WifiConfigManager::getPortalName() const {
  return String(portalName_);
}

void WifiConfigManager::printStatus(Print& out) const {
  out.printf("[WIFI] Status : %s\n", statusToString(status_));
  out.printf("[WIFI] SSID   : %s\n", WiFi.SSID().c_str());

  if (WiFi.status() == WL_CONNECTED) {
    out.printf("[WIFI] IP     : %s\n", WiFi.localIP().toString().c_str());
  } else {
    out.printf("[WIFI] Portal : %s\n", portalName_);
  }
}

const char* WifiConfigManager::statusToString(Status status) {
  switch (status) {
    case Status::Idle:
      return "Idle";
    case Status::Connecting:
      return "Connecting";
    case Status::PortalActive:
      return "Portal Active";
    case Status::Connected:
      return "Connected";
    case Status::Disconnected:
      return "Disconnected";
    default:
      return "Unknown";
  }
}

void WifiConfigManager::buildPortalName() {
  const char* prefix =
      (config_.portalNamePrefix != nullptr && config_.portalNamePrefix[0] != '\0')
          ? config_.portalNamePrefix
          : "PVMonitor";

  uint8_t mac[6] = {};
  WiFi.macAddress(mac);

  std::snprintf(portalName_,
                sizeof(portalName_),
                "%s-%02X%02X%02X",
                prefix,
                static_cast<unsigned>(mac[3]),
                static_cast<unsigned>(mac[4]),
                static_cast<unsigned>(mac[5]));
}

void WifiConfigManager::applySettings() {
  manager_.setDebugOutput(config_.enableDebugOutput);
  manager_.setConnectTimeout(config_.connectTimeoutSeconds);
  manager_.setConfigPortalTimeout(config_.portalTimeoutSeconds);
}

void WifiConfigManager::setStatus(Status status) {
  status_ = status;
}

bool WifiConfigManager::waitForReconnect() {
  if (WiFi.SSID().length() == 0U) {
    return false;
  }

  setStatus(Status::Connecting);

  WiFi.mode(WIFI_STA);
  WiFi.begin();

  const unsigned long startedAt = millis();
  while ((millis() - startedAt) < config_.reconnectTimeoutMs) {
    if (WiFi.status() == WL_CONNECTED) {
      return true;
    }

    delay(250);
  }

  return false;
}

bool WifiConfigManager::startConfigPortal() {
  setStatus(Status::PortalActive);

  const bool usePassword =
      (config_.portalPassword != nullptr) && (config_.portalPassword[0] != '\0');

  const bool connected =
      usePassword ? manager_.autoConnect(portalName_, config_.portalPassword)
                  : manager_.autoConnect(portalName_);

  setStatus(connected ? Status::Connected : Status::Disconnected);
  return connected;
}

}  // namespace WifiModule

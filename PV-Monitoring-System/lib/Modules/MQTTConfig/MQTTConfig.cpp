#include "MQTTConfig.h"

#include <cstdio>
#include <cstring>

namespace MQTTModule {

namespace {

bool hasValue(const char* value) {
  return value != nullptr && value[0] != '\0';
}

void maskValue(const char* value, char* buffer, size_t bufferSize) {
  if (buffer == nullptr || bufferSize == 0U) {
    return;
  }

  buffer[0] = '\0';
  if (!hasValue(value)) {
    return;
  }

  const size_t valueLength = std::strlen(value);
  if (valueLength > 4U) {
    std::snprintf(buffer, bufferSize, "****%s", value + valueLength - 4U);
  } else {
    std::snprintf(buffer, bufferSize, "****");
  }
}

}  // namespace

MQTTConfigManager::MQTTConfigManager() : mqttClient_(wifiClient_) {}

bool MQTTConfigManager::begin(const Config& config) {
  config_ = config;
  configureClient();
  status_ = mqttClient_.connected() ? Status::Connected : Status::Disconnected;
  initialized_ = true;
  return true;
}

bool MQTTConfigManager::ensureConnected() {
  if (!initialized_ && !begin()) {
    return false;
  }

  if (mqttClient_.connected()) {
    setStatus(Status::Connected);
    return true;
  }

  if (WiFi.status() != WL_CONNECTED) {
    setStatus(Status::NoWifi);
    return false;
  }

  if (!canAttemptReconnect()) {
    return false;
  }

  setStatus(Status::Connecting);
  lastConnectAttemptMs_ = millis();

  const bool connected =
      hasValue(config_.username)
          ? mqttClient_.connect(config_.clientId, config_.username, config_.password)
          : mqttClient_.connect(config_.clientId);
  setStatus(connected ? Status::Connected : Status::ConnectFailed);
  return connected;
}

void MQTTConfigManager::loop() {
  if (ensureConnected()) {
    mqttClient_.loop();
  }
}

void MQTTConfigManager::disconnect() {
  mqttClient_.disconnect();
  setStatus(Status::Disconnected);
}

bool MQTTConfigManager::isConnected() {
  return mqttClient_.connected();
}

Status MQTTConfigManager::getStatus() const {
  return status_;
}

int8_t MQTTConfigManager::getClientState() {
  return mqttClient_.state();
}

Config MQTTConfigManager::getConfig() const {
  return config_;
}

bool MQTTConfigManager::publishJson(const char* topic, const char* jsonPayload, bool retained) {
  if (!hasValue(topic) || !hasValue(jsonPayload)) {
    return false;
  }

  if (!ensureConnected()) {
    return false;
  }

  const bool published = mqttClient_.publish(topic, jsonPayload, retained);
  if (!published) {
    setStatus(Status::PublishFailed);
  }

  return published;
}

bool MQTTConfigManager::publishDataJson(const char* jsonPayload, bool retained) {
  return publishJson(config_.dataTopic, jsonPayload, retained);
}

bool MQTTConfigManager::publishInfoJson(const char* jsonPayload, bool retained) {
  return publishJson(config_.infoTopic, jsonPayload, retained);
}

bool MQTTConfigManager::publishData(const char* key, float value, uint8_t decimals) {
  char numberBuffer[32] = {};
  std::snprintf(numberBuffer, sizeof(numberBuffer), "%.*f", decimals, value);
  return publishKeyValue(config_.dataTopic, key, numberBuffer, false);
}

bool MQTTConfigManager::publishData(const char* key, int32_t value) {
  char numberBuffer[32] = {};
  std::snprintf(numberBuffer, sizeof(numberBuffer), "%ld", static_cast<long>(value));
  return publishKeyValue(config_.dataTopic, key, numberBuffer, false);
}

bool MQTTConfigManager::publishData(const char* key, uint32_t value) {
  char numberBuffer[32] = {};
  std::snprintf(numberBuffer, sizeof(numberBuffer), "%lu", static_cast<unsigned long>(value));
  return publishKeyValue(config_.dataTopic, key, numberBuffer, false);
}

bool MQTTConfigManager::publishData(const char* key, bool value) {
  return publishKeyValue(config_.dataTopic, key, value ? "true" : "false", false);
}

bool MQTTConfigManager::publishData(const char* key, const char* value) {
  return publishKeyValue(config_.dataTopic, key, value, true);
}

bool MQTTConfigManager::publishInfo(const char* key, float value, uint8_t decimals) {
  char numberBuffer[32] = {};
  std::snprintf(numberBuffer, sizeof(numberBuffer), "%.*f", decimals, value);
  return publishKeyValue(config_.infoTopic, key, numberBuffer, false);
}

bool MQTTConfigManager::publishInfo(const char* key, int32_t value) {
  char numberBuffer[32] = {};
  std::snprintf(numberBuffer, sizeof(numberBuffer), "%ld", static_cast<long>(value));
  return publishKeyValue(config_.infoTopic, key, numberBuffer, false);
}

bool MQTTConfigManager::publishInfo(const char* key, uint32_t value) {
  char numberBuffer[32] = {};
  std::snprintf(numberBuffer, sizeof(numberBuffer), "%lu", static_cast<unsigned long>(value));
  return publishKeyValue(config_.infoTopic, key, numberBuffer, false);
}

bool MQTTConfigManager::publishInfo(const char* key, bool value) {
  return publishKeyValue(config_.infoTopic, key, value ? "true" : "false", false);
}

bool MQTTConfigManager::publishInfo(const char* key, const char* value) {
  return publishKeyValue(config_.infoTopic, key, value, true);
}

PubSubClient& MQTTConfigManager::client() {
  return mqttClient_;
}

void MQTTConfigManager::printStatus(Print& out) {
  char maskedPassword[20] = {};
  maskValue(config_.password, maskedPassword, sizeof(maskedPassword));

  out.printf("[MQTT] Status      : %s\n", statusToString(status_));
  out.printf("[MQTT] Broker      : %s:%u\n", config_.server, config_.port);
  out.printf("[MQTT] Client ID   : %s\n", config_.clientId);
  out.printf("[MQTT] Username    : %s\n", hasValue(config_.username) ? config_.username : "-");
  out.printf("[MQTT] Password    : %s\n", hasValue(config_.password) ? maskedPassword : "-");
  out.printf("[MQTT] Data Topic  : %s\n", hasValue(config_.dataTopic) ? config_.dataTopic : "-");
  out.printf("[MQTT] Info Topic  : %s\n", hasValue(config_.infoTopic) ? config_.infoTopic : "-");
  out.printf("[MQTT] WiFi        : %s\n", (WiFi.status() == WL_CONNECTED) ? "Connected" : "Not Connected");
  out.printf("[MQTT] ClientState : %d (%s)\n",
             static_cast<int>(mqttClient_.state()),
             clientStateToString(mqttClient_.state()));
}

const char* MQTTConfigManager::statusToString(Status status) {
  switch (status) {
    case Status::Idle:
      return "Idle";
    case Status::NoWifi:
      return "No WiFi";
    case Status::Connecting:
      return "Connecting";
    case Status::Connected:
      return "Connected";
    case Status::ConnectFailed:
      return "Connect Failed";
    case Status::Disconnected:
      return "Disconnected";
    case Status::PublishFailed:
      return "Publish Failed";
    default:
      return "Unknown";
  }
}

const char* MQTTConfigManager::clientStateToString(int8_t state) {
  switch (state) {
    case -4:
      return "Connection Timeout";
    case -3:
      return "Connection Lost";
    case -2:
      return "Connect Failed";
    case -1:
      return "Disconnected";
    case 0:
      return "Connected";
    case 1:
      return "Bad Protocol";
    case 2:
      return "Bad Client ID";
    case 3:
      return "Unavailable";
    case 4:
      return "Bad Credentials";
    case 5:
      return "Unauthorized";
    default:
      return "Unknown";
  }
}

void MQTTConfigManager::configureClient() {
  mqttClient_.setServer(config_.server, config_.port);
  mqttClient_.setKeepAlive(config_.keepAliveSeconds);
  mqttClient_.setSocketTimeout(config_.socketTimeoutSeconds);
  mqttClient_.setBufferSize(config_.bufferSize);
}

void MQTTConfigManager::setStatus(Status status) {
  status_ = status;
}

bool MQTTConfigManager::publishKeyValue(const char* topic,
                                        const char* key,
                                        const char* value,
                                        bool quoteValue) {
  if (!hasValue(topic) || !hasValue(key) || value == nullptr) {
    return false;
  }

  char payloadBuffer[kPayloadBufferSize] = {};
  const int written = std::snprintf(payloadBuffer,
                                    sizeof(payloadBuffer),
                                    quoteValue ? "{\"%s\":\"%s\"}" : "{\"%s\":%s}",
                                    key,
                                    value);

  if (written <= 0 || written >= static_cast<int>(sizeof(payloadBuffer))) {
    return false;
  }

  return publishJson(topic, payloadBuffer, false);
}

bool MQTTConfigManager::canAttemptReconnect() const {
  if (lastConnectAttemptMs_ == 0U) {
    return true;
  }

  return (millis() - lastConnectAttemptMs_) >= config_.reconnectIntervalMs;
}

}  // namespace MQTTModule

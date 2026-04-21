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

MQTTConfigManager* MQTTConfigManager::instance_ = nullptr;

MQTTConfigManager::MQTTConfigManager() : mqttClient_(wifiClient_) {
  instance_ = this;
}

bool MQTTConfigManager::begin(const Config& config) {
  config_ = config;
  buildTopics();
  configureClient();
  status_ = mqttClient_.connected() ? Status::Connected : Status::Disconnected;
  initialized_ = true;
  subscribed_ = false;
  return true;
}

bool MQTTConfigManager::ensureConnected() {
  if (!initialized_ && !begin()) {
    return false;
  }

  if (mqttClient_.connected()) {
    setStatus(Status::Connected);
    ensureSubscribed();
    return true;
  }

  subscribed_ = false;

  if (WiFi.status() != WL_CONNECTED) {
    setStatus(Status::NoWifi);
    return false;
  }

  if (!canAttemptReconnect()) {
    return false;
  }

  setStatus(Status::Connecting);
  lastConnectAttemptMs_ = millis();

  const char* clientId = effectiveClientId();
  const bool connected =
      hasValue(config_.username)
          ? mqttClient_.connect(clientId, config_.username, config_.password)
          : mqttClient_.connect(clientId);

  setStatus(connected ? Status::Connected : Status::ConnectFailed);

  if (connected) {
    ensureSubscribed();
  }

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
  subscribed_ = false;
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

const char* MQTTConfigManager::getPubsTopic() const {
  return pubsTopic_;
}

const char* MQTTConfigManager::getSubsTopic() const {
  return subsTopic_;
}

const char* MQTTConfigManager::getStatusTopic() const {
  return statusTopic_;
}

const char* MQTTConfigManager::getDeviceId() const {
  return hasValue(config_.deviceId) ? config_.deviceId : kDefaultDeviceId;
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

bool MQTTConfigManager::publishPubs(const char* jsonPayload, bool retained) {
  return publishJson(pubsTopic_, jsonPayload, retained);
}

bool MQTTConfigManager::publishStatus(const char* jsonPayload, bool retained) {
  return publishJson(statusTopic_, jsonPayload, retained);
}

void MQTTConfigManager::setMessageCallback(MessageCallback callback) {
  messageCallback_ = std::move(callback);
  mqttClient_.setCallback(&MQTTConfigManager::internalCallback);
}

PubSubClient& MQTTConfigManager::client() {
  return mqttClient_;
}

void MQTTConfigManager::printStatus(Print& out) {
  char maskedPassword[20] = {};
  maskValue(config_.password, maskedPassword, sizeof(maskedPassword));

  out.printf("[MQTT] Status      : %s\n", statusToString(status_));
  out.printf("[MQTT] Broker      : %s:%u\n", config_.server, config_.port);
  out.printf("[MQTT] Device ID   : %s\n", getDeviceId());
  out.printf("[MQTT] Client ID   : %s\n", effectiveClientId());
  out.printf("[MQTT] Username    : %s\n", hasValue(config_.username) ? config_.username : "-");
  out.printf("[MQTT] Password    : %s\n", hasValue(config_.password) ? maskedPassword : "-");
  out.printf("[MQTT] Pubs Topic  : %s\n", pubsTopic_);
  out.printf("[MQTT] Subs Topic  : %s\n", subsTopic_);
  out.printf("[MQTT] Status Topic: %s\n", statusTopic_);
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

void MQTTConfigManager::buildTopics() {
  const char* prefix = hasValue(config_.topicPrefix) ? config_.topicPrefix : kDefaultTopicPrefix;
  const char* deviceId = getDeviceId();

  std::snprintf(pubsTopic_, sizeof(pubsTopic_), "%s/%s/%s", prefix, deviceId, kPubsSuffix);
  std::snprintf(subsTopic_, sizeof(subsTopic_), "%s/%s/%s", prefix, deviceId, kSubsSuffix);
  std::snprintf(statusTopic_, sizeof(statusTopic_), "%s/%s/%s", prefix, deviceId, kStatusSuffix);
}

void MQTTConfigManager::setStatus(Status status) {
  status_ = status;
}

bool MQTTConfigManager::ensureSubscribed() {
  if (subscribed_ || !hasValue(subsTopic_) || !mqttClient_.connected()) {
    return subscribed_;
  }

  subscribed_ = mqttClient_.subscribe(subsTopic_, config_.subscribeQos);
  return subscribed_;
}

bool MQTTConfigManager::canAttemptReconnect() const {
  if (lastConnectAttemptMs_ == 0U) {
    return true;
  }

  return (millis() - lastConnectAttemptMs_) >= config_.reconnectIntervalMs;
}

const char* MQTTConfigManager::effectiveClientId() const {
  if (hasValue(config_.clientId)) {
    return config_.clientId;
  }
  return getDeviceId();
}

void MQTTConfigManager::internalCallback(char* topic, uint8_t* payload, unsigned int length) {
  if (instance_ == nullptr || !instance_->messageCallback_) {
    return;
  }
  instance_->messageCallback_(topic, payload, length);
}

}  // namespace MQTTModule

#pragma once

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFi.h>

#include <functional>

namespace MQTTModule {

static constexpr char kDefaultServer[] = "broker.hivemq.com";
static constexpr uint16_t kDefaultPort = 1883;
static constexpr char kDefaultDeviceId[] = "pv-monitoring-01";
static constexpr char kDefaultTopicPrefix[] = "pv-monitoring";
static constexpr char kDefaultUsername[] = "";
static constexpr char kDefaultPassword[] = "";
static constexpr char kPubsSuffix[] = "pubs";
static constexpr char kSubsSuffix[] = "subs";
static constexpr char kStatusSuffix[] = "status";

enum class Status : uint8_t {
  Idle = 0,
  NoWifi,
  Connecting,
  Connected,
  ConnectFailed,
  Disconnected,
  PublishFailed,
};

struct Config {
  const char* server = kDefaultServer;
  uint16_t port = kDefaultPort;
  const char* deviceId = kDefaultDeviceId;
  const char* topicPrefix = kDefaultTopicPrefix;
  const char* clientId = nullptr;  // kalau null, pakai deviceId
  const char* username = kDefaultUsername;
  const char* password = kDefaultPassword;
  uint16_t keepAliveSeconds = 60;
  uint16_t socketTimeoutSeconds = 15;
  uint16_t bufferSize = 1024;
  uint32_t reconnectIntervalMs = 5000;
  uint8_t subscribeQos = 0;
};

using MessageCallback =
    std::function<void(const char* topic, const uint8_t* payload, unsigned int length)>;

class MQTTConfigManager {
 public:
  MQTTConfigManager();

  bool begin(const Config& config = Config{});
  bool ensureConnected();
  void loop();
  void disconnect();

  bool isConnected();
  Status getStatus() const;
  int8_t getClientState();
  Config getConfig() const;

  const char* getPubsTopic() const;
  const char* getSubsTopic() const;
  const char* getStatusTopic() const;
  const char* getDeviceId() const;

  bool publishJson(const char* topic, const char* jsonPayload, bool retained = false);
  bool publishPubs(const char* jsonPayload, bool retained = false);
  bool publishStatus(const char* jsonPayload, bool retained = false);

  void setMessageCallback(MessageCallback callback);

  PubSubClient& client();

  void printStatus(Print& out);
  static const char* statusToString(Status status);
  static const char* clientStateToString(int8_t state);

 private:
  static constexpr size_t kMaxTopicLength = 128;

  WiFiClient wifiClient_{};
  PubSubClient mqttClient_;
  Config config_{};
  Status status_ = Status::Idle;
  bool initialized_ = false;
  bool subscribed_ = false;
  unsigned long lastConnectAttemptMs_ = 0;

  char pubsTopic_[kMaxTopicLength] = {};
  char subsTopic_[kMaxTopicLength] = {};
  char statusTopic_[kMaxTopicLength] = {};

  MessageCallback messageCallback_{};

  static MQTTConfigManager* instance_;
  static void internalCallback(char* topic, uint8_t* payload, unsigned int length);

  void configureClient();
  void buildTopics();
  void setStatus(Status status);
  bool ensureSubscribed();
  bool canAttemptReconnect() const;
  const char* effectiveClientId() const;
};

}  // namespace MQTTModule

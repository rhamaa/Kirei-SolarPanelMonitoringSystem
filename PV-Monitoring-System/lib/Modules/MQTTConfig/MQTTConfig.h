#pragma once

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFi.h>

namespace MQTTModule {

static constexpr char kDefaultServer[] = "broker.hivemq.com";
static constexpr uint16_t kDefaultPort = 1883;
static constexpr char kDefaultClientId[] = "pv-monitoring-client";
static constexpr char kDefaultUsername[] = "";
static constexpr char kDefaultPassword[] = "";
static constexpr char kDefaultDataTopic[] = "pv-monitoring/data";
static constexpr char kDefaultInfoTopic[] = "pv-monitoring/info";

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
  const char* clientId = kDefaultClientId;
  const char* username = kDefaultUsername;
  const char* password = kDefaultPassword;
  const char* dataTopic = kDefaultDataTopic;
  const char* infoTopic = kDefaultInfoTopic;
  uint16_t keepAliveSeconds = 60;
  uint16_t socketTimeoutSeconds = 15;
  uint16_t bufferSize = 1024;
  uint32_t reconnectIntervalMs = 5000;
};

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

  bool publishJson(const char* topic, const char* jsonPayload, bool retained = false);
  bool publishDataJson(const char* jsonPayload, bool retained = false);
  bool publishInfoJson(const char* jsonPayload, bool retained = false);

  bool publishData(const char* key, float value, uint8_t decimals = 2);
  bool publishData(const char* key, int32_t value);
  bool publishData(const char* key, uint32_t value);
  bool publishData(const char* key, bool value);
  bool publishData(const char* key, const char* value);

  bool publishInfo(const char* key, float value, uint8_t decimals = 2);
  bool publishInfo(const char* key, int32_t value);
  bool publishInfo(const char* key, uint32_t value);
  bool publishInfo(const char* key, bool value);
  bool publishInfo(const char* key, const char* value);

  PubSubClient& client();

  void printStatus(Print& out);
  static const char* statusToString(Status status);
  static const char* clientStateToString(int8_t state);

 private:
  static constexpr size_t kPayloadBufferSize = 256;

  WiFiClient wifiClient_{};
  PubSubClient mqttClient_;
  Config config_{};
  Status status_ = Status::Idle;
  bool initialized_ = false;
  unsigned long lastConnectAttemptMs_ = 0;

  void configureClient();
  void setStatus(Status status);
  bool publishKeyValue(const char* topic, const char* key, const char* value, bool quoteValue);
  bool canAttemptReconnect() const;
};

}  // namespace MQTTModule

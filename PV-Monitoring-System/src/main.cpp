#include <Arduino.h>
#include <SendDataTask.h>
#include <WifiTask.h>
#include <dataTask.h>

namespace {

constexpr char kDeviceId[] = "pv-monitoring-01";
constexpr char kFirmwareVersion[] = "1.0.0";

constexpr char kMqttServer[] = "broker.hivemq.com";
constexpr uint16_t kMqttPort = 1883;
constexpr char kMqttUsername[] = "";
constexpr char kMqttPassword[] = "";

constexpr char kInfluxUrl[] = "http://192.168.68.106:8086";
constexpr char kInfluxOrg[] = "YOUR_ORG";
constexpr char kInfluxBucket[] = "YOUR_BUCKET";
constexpr char kInfluxToken[] = "YOUR_INFLUXDB_V2_TOKEN";
constexpr char kInfluxMeasurement[] = "pv_monitoring";

SendDataTask::Config buildSendConfig() {
  SendDataTask::Config config{};

  config.firmwareVersion = kFirmwareVersion;
  config.publishIntervalMs = 10000;
  config.statusIntervalMs = 30000;
  config.publishStatusOnConnect = true;

  config.mqttConfig.server = kMqttServer;
  config.mqttConfig.port = kMqttPort;
  config.mqttConfig.deviceId = kDeviceId;
  config.mqttConfig.topicPrefix = "pv-monitoring";
  config.mqttConfig.username = kMqttUsername;
  config.mqttConfig.password = kMqttPassword;

  config.influxConfig.url = kInfluxUrl;
  config.influxConfig.org = kInfluxOrg;
  config.influxConfig.bucket = kInfluxBucket;
  config.influxConfig.token = kInfluxToken;
  config.influxConfig.measurement = kInfluxMeasurement;
  config.influxConfig.enabled = true;

  return config;
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(1000);

  if (!WifiTask::begin()) {
    Serial.println("[SYSTEM] Gagal membuat task WiFi.");
  }

  if (!DataTask::begin()) {
    Serial.println("[SYSTEM] Gagal membuat task PV data.");
  }

  if (!SendDataTask::begin(buildSendConfig())) {
    Serial.println("[SYSTEM] Gagal membuat task kirim data.");
  }
}

void loop() {
  if (Serial.available()) {
    const char command = static_cast<char>(Serial.read());
    if ((command == 'R') || (command == 'r')) {
      if (DataTask::resetInverterEnergy()) {
        Serial.println("[INV] Energy counter direset ke 0 kWh");
      } else {
        Serial.println("[INV] Gagal reset energy counter.");
      }
    }
  }

  delay(100);
}

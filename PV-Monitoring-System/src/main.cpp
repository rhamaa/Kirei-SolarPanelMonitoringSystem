#include <Arduino.h>
#include <SendDataTask.h>
#include <WifiTask.h>
#include <dataTask.h>

namespace {

constexpr char kDeviceId[] = "pv-monitoring-01";
constexpr char kFirmwareVersion[] = "1.0.0";

// RabbitMQ: 15672 = Management UI; MQTT = 1883 (plugin rabbitmq_mqtt).
constexpr char kMqttServer[] = "192.168.68.106";
constexpr uint16_t kMqttPort = 1883;
constexpr char kMqttUsername[] = "mqtt_user";
constexpr char kMqttPassword[] = "mqtt_password";

// InfluxDB v2 — sama pola dengan contoh resmi InfluxDbClient (HTTP lokal, tanpa cert Cloud).
constexpr char kInfluxUrl[] = "http://192.168.68.106:8086";
// Boleh org **nama** atau **org ID** hex dari UI Influx.
constexpr char kInfluxOrg[] = "86d3a746830ba285";
constexpr char kInfluxBucket[] = "pv-monitoring";
constexpr char kInfluxToken[] =
    "qg9E8G99bpb3AIN3YI_oottr6gX8eV9Q0nFk_RIZRAhT8rD3mX76R26NsGk7nM1CqqjuoKnNyHtw7FdN2YF3OA==";
constexpr char kInfluxMeasurement[] = "pv_monitoring";
/// POSIX TZ untuk `timeSync()` (contoh Influx Arduino: "UTC7").
constexpr char kInfluxTzInfo[] = "UTC7";
constexpr bool kInfluxEnabled = true;

SendDataTask::Config buildSendConfig() {
  SendDataTask::Config config{};

  config.firmwareVersion = kFirmwareVersion;
  config.publishIntervalMs = 5000;
  config.statusIntervalMs = 30000;
  config.publishStatusOnConnect = true;

  config.mqttConfig.server = kMqttServer;
  config.mqttConfig.port = kMqttPort;
  config.mqttConfig.deviceId = kDeviceId;
  config.mqttConfig.topicPrefix = "pv-monitoring";
  config.mqttConfig.username = kMqttUsername;
  config.mqttConfig.password = kMqttPassword;

  config.influx.url = kInfluxUrl;
  config.influx.org = kInfluxOrg;
  config.influx.bucket = kInfluxBucket;
  config.influx.token = kInfluxToken;
  config.influx.measurement = kInfluxMeasurement;
  config.influx.tzInfo = kInfluxTzInfo;
  config.influx.enabled = kInfluxEnabled;

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

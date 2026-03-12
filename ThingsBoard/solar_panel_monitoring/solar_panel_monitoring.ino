#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiManager.h>
#include <esp_system.h>

// WiFi credentials
const char* ap_name = "SolarPanel-Setup";

// ThingsBoard Cloud MQTT broker
const char* mqtt_server = "mqtt.thingsboard.cloud";
const int mqtt_port = 1883; // gunakan 8883 jika TLS
const char* token = "Qtuw3WmUMDKXJyjbEuc8"; // ambil dari ThingsBoard Cloud
const char* client_id = "l341yk9s5cqaz75u6sp1";

WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi() {
  WiFi.mode(WIFI_STA);
  WiFiManager wm;
  wm.setConfigPortalTimeout(180);
  bool ok = wm.autoConnect(ap_name);
  if (!ok) {
    delay(3000);
    ESP.restart();
  }
  Serial.println("WiFi connected");
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(client_id, token, NULL)) {
      Serial.println("Connected to ThingsBoard Cloud");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  randomSeed(esp_random());
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Simulasi data panel surya
  float solarPower = random(0, 2001) / 10.0;
  float voltage = random(120, 251) / 10.0;
  float amper = random(0, 121) / 10.0;
  int batteryStatus = random(0, 101);
  float outputPower = random(0, 2001) / 10.0;

  // Buat payload JSON
  String payload = "{";
  payload += "\"solarPower\":" + String(solarPower) + ",";
  payload += "\"voltage\":" + String(voltage) + ",";
  payload += "\"amper\":" + String(amper) + ",";
  payload += "\"batteryStatus\":" + String(batteryStatus) + ",";
  payload += "\"outputPower\":" + String(outputPower);
  payload += "}";

  // Publish ke ThingsBoard Cloud
  if (client.publish("v1/devices/me/telemetry", payload.c_str())) {
    Serial.println("Data sent: " + payload);
  } else {
    Serial.println("Publish failed");
  }

  delay(5000); // kirim tiap 5 detik
}
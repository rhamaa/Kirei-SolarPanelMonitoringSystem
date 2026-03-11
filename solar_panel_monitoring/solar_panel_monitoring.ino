#include <WiFi.h>
#include <PubSubClient.h>

// WiFi credentials
const char* ssid = "Kirei";
const char* password = "berakitkehulu";

// ThingsBoard Cloud MQTT broker
const char* mqtt_server = "mqtt.thingsboard.cloud";
const int mqtt_port = 1883; // gunakan 8883 jika TLS
const char* token = "eeE1oiwsBRyV9MpzY4fE"; // ambil dari ThingsBoard Cloud
const char* client_id = "l341yk9s5cqaz75u6sp1";

WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
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
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Simulasi data panel surya
  float solarPower = 120.5;   // Watt
  float voltage = 18.2;       // Volt
  float amper = 6.6;          // Ampere
  int batteryStatus = 75;     // %
  float outputPower = 95.3;   // Watt

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
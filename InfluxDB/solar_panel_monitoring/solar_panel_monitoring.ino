#if defined(ESP32)
  #include <WiFi.h>
  #include <WiFiManager.h>
  #include <esp_system.h>
  #define DEVICE "ESP32"
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <ESP8266WiFiMulti.h>
  #define DEVICE "ESP8266"
#endif

#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>

const char* ap_name = "SolarPanel-Setup";

#define INFLUXDB_URL "https://us-east-1-1.aws.cloud2.influxdata.com"
#define INFLUXDB_TOKEN "sBkCHPHd8Z33QKgwy-q2iN5REXMyOAM0GpCg8uMdEOgU1JICef2FQ-_JHeVF13bdbsHbQ7-JTk8pYmkCO6knOA=="
#define INFLUXDB_ORG "06e24cfe47d23d33"
#define INFLUXDB_BUCKET "panel-surya-monitoring"
#define TZ_INFO "UTC7"

InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);
Point sensor("solar_panel_monitoring");

void setup_wifi() {
#if defined(ESP32)
  WiFi.mode(WIFI_STA);
  WiFiManager wm;
  wm.setConfigPortalTimeout(180);
  bool ok = wm.autoConnect(ap_name);
  if (!ok) {
    delay(3000);
    ESP.restart();
  }
  Serial.println("WiFi connected");
#endif
}

void setup() {
  Serial.begin(115200);
  randomSeed(esp_random());
  setup_wifi();
  timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");
  if (client.validateConnection()) {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(client.getServerUrl());
  } else {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(client.getLastErrorMessage());
  }
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    setup_wifi();
  }
  float solarPower = random(0, 2001) / 10.0;
  float voltage = random(120, 251) / 10.0;
  float amper = random(0, 121) / 10.0;
  int batteryStatus = random(0, 101);
  float outputPower = random(0, 2001) / 10.0;

  sensor.clearFields();
  sensor.clearTags();
  sensor.addTag("device", DEVICE);
  sensor.addField("solarPower", solarPower);
  sensor.addField("voltage", voltage);
  sensor.addField("amper", amper);
  sensor.addField("batteryStatus", batteryStatus);
  sensor.addField("outputPower", outputPower);

  if (client.writePoint(sensor)) {
    Serial.print("Data sent to InfluxDB: ");
    Serial.println(sensor.toLineProtocol());
  } else {
    Serial.print("InfluxDB write failed: ");
    Serial.println(client.getLastErrorMessage());
  }

  delay(5000);
}
#include <Arduino.h>
#include <SendDataTask.h>
#include <WifiTask.h>
#include <dataTask.h>

void setup() {
  Serial.begin(115200);
  delay(1000);

  if (!WifiTask::begin()) {
    Serial.println("[SYSTEM] Gagal membuat task WiFi.");
  }

  if (!DataTask::begin()) {
    Serial.println("[SYSTEM] Gagal membuat task PV data.");
  }

  if (!SendDataTask::begin()) {
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

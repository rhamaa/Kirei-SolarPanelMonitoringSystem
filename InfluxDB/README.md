# Solar Panel Monitoring (InfluxDB) — R&D

ESP32 sketch that sends simulated solar telemetry to InfluxDB Cloud using HTTPS/TLS. WiFi is provisioned with WiFiManager (captive portal, no hardcoded SSID/password).

## Struktur
- Sketch: `solar_panel_monitoring/solar_panel_monitoring.ino`
- Measurement: `solar_panel_monitoring`
- Fields: `solarPower`, `voltage`, `amper`, `batteryStatus`, `outputPower`
- Tag: `device=ESP32`

## Prasyarat
- Board: ESP32 (Arduino framework)
- Library: `WiFiManager`, `InfluxDB Client for Arduino`
- InfluxDB Cloud credential:
  - URL: `https://us-east-1-1.aws.cloud2.influxdata.com`
  - ORG: `06e24cfe47d23d33`
  - BUCKET: `panel-surya-monitoring`
  - TOKEN: (gunakan token kamu; jangan commit token asli)

## Cara pakai (flash & WiFi)
1) Install library di Arduino IDE:
   - `WiFiManager`
   - `InfluxDB Client for Arduino`
2) Buka `solar_panel_monitoring/solar_panel_monitoring.ino` dan isi konstanta:
   - `INFLUXDB_URL`, `INFLUXDB_ORG`, `INFLUXDB_BUCKET`, `INFLUXDB_TOKEN`
3) Upload ke ESP32.
4) Provisioning WiFi via WiFiManager:
   - ESP32 buat AP: `SolarPanel-Setup`
   - Connect dari ponsel/laptop, buka `http://192.168.4.1`
   - Pilih SSID & password WiFi, simpan
5) Setelah tersambung, sketch akan:
   - `timeSync()` (wajib untuk TLS)
   - Validasi koneksi InfluxDB
   - Kirim telemetry tiap 5 detik (randomized untuk R&D)

## Cek data di InfluxDB Cloud (Data Explorer)
1) Buka **Data Explorer** di InfluxDB Cloud.
2) Pilih Bucket: `panel-surya-monitoring`.
3) Measurement: `solar_panel_monitoring`.
4) Tag (opsional): `device = ESP32`.
5) Fields: `solarPower`, `voltage`, `amper`, `batteryStatus`, `outputPower`.
6) Set rentang waktu (mis. `Past 1h`) lalu Submit.

### Contoh query Flux (Script Editor)
```flux
from(bucket: "panel-surya-monitoring")
  |> range(start: -1h)
  |> filter(fn: (r) => r._measurement == "solar_panel_monitoring")
  |> filter(fn: (r) => r.device == "ESP32")
  |> filter(fn: (r) => r._field == "solarPower" or r._field == "voltage" or r._field == "amper" or r._field == "batteryStatus" or r._field == "outputPower")
  |> yield(name: "telemetry")
```

## Troubleshooting singkat
- **read Timeout / connection refused**: cek WiFi/Internet, coba hotspot lain; pastikan port 443 tidak diblok; pastikan `timeSync` sukses.
- **Unauthorized**: salah token/org/bucket atau URL; cek lagi kredensial.
- **Waktu meleset**: `timeSync` wajib sukses untuk TLS; pastikan NTP bisa diakses.

## Keamanan
- Token InfluxDB adalah rahasia. Jangan commit token asli. Tambahkan file rahasia ke `.gitignore` bila perlu.

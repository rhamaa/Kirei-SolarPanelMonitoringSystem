# PV Monitoring System

Firmware ESP32 untuk membaca data MPPT dan inverter, lalu mengirimkannya ke broker MQTT menggunakan `PubSubClient` dan ke **InfluxDB v2** lewat HTTP.

## Ringkasan

Sistem ini menjalankan tiga task utama:

1. `WifiTask` untuk koneksi WiFi
2. `DataTask` untuk membaca sensor MPPT dan inverter
3. `SendDataTask` untuk publish data ke MQTT **dan** menulis ke InfluxDB

Board target saat ini ada di `platformio.ini`:

- Environment: `4d_systems_esp32s3_gen4_r8n16`
- Framework: `Arduino`

## Library yang dipakai

- `PubSubClient`
- `WiFiManager`
- `PZEM-004T-v30`
- `HTTPClient` (bawaan ESP32 Arduino core)

## Konfigurasi Default

Lihat `src/main.cpp` untuk konstanta yang perlu diisi sebelum flashing:

| Konstanta | Keterangan |
| --- | --- |
| `kDeviceId` | ID perangkat, dipakai di seluruh topic MQTT (`pv-monitoring/<kDeviceId>/...`) |
| `kFirmwareVersion` | Versi firmware, dikirim pada payload status |
| `kMqttServer` / `kMqttPort` | Alamat broker MQTT |
| `kMqttUsername` / `kMqttPassword` | Kredensial MQTT (boleh string kosong) |
| `kInfluxUrl` | Base URL InfluxDB v2, contoh `http://192.168.68.106:8086` |
| `kInfluxOrg` | Nama organisasi InfluxDB v2 |
| `kInfluxBucket` | Bucket tujuan |
| `kInfluxToken` | API token InfluxDB v2 dengan permission write ke bucket |
| `kInfluxMeasurement` | Nama measurement (default `pv_monitoring`) |

## Daftar Topic MQTT

Firmware ini **bukan** integrasi Home Assistant. Hanya ada tiga topic yang semuanya dibentuk dari `pv-monitoring/<DeviceID>/...`.

Contoh dengan `DeviceID = pv-monitoring-01`:

| Topic | Arah | Fungsi |
| --- | --- | --- |
| `pv-monitoring/pv-monitoring-01/pubs` | Device -> Broker | Publish data sensor MPPT + inverter periodik (default 10 s) |
| `pv-monitoring/pv-monitoring-01/subs` | Broker -> Device | Device subscribe di topic ini untuk menerima command |
| `pv-monitoring/pv-monitoring-01/status` | Device -> Broker | Publish status device (IP, RSSI, uptime, firmware) saat konek dan periodik (default 30 s) |

### 1. Topic pubs - payload data sensor

Contoh payload JSON yang dipublish ke `pv-monitoring/<DeviceID>/pubs`:

```json
{
  "device_id": "pv-monitoring-01",
  "mppt_valid": true,
  "mppt_pv_voltage": 18.5,
  "mppt_charging_power": 120.0,
  "mppt_charging_current": 6.48,
  "mppt_battery_voltage": 13.8,
  "mppt_charging_status": 0,
  "mppt_charging_status_text": "MPPT Charging",
  "mppt_load_current": 1.25,
  "mppt_load_power": 17.3,
  "mppt_fault_code": 0,
  "mppt_timestamp_ms": 123456,
  "inverter_valid": true,
  "inverter_ac_voltage": 220.4,
  "inverter_ac_current": 0.431,
  "inverter_ac_power": 92.5,
  "inverter_ac_energy": 1.234,
  "inverter_ac_frequency": 50.0,
  "inverter_ac_power_factor": 0.97,
  "inverter_ac_apparent_power": 95.4,
  "inverter_timestamp_ms": 123400,
  "wifi_rssi": -58
}
```

### 2. Topic status - payload status device

Contoh payload JSON yang dipublish ke `pv-monitoring/<DeviceID>/status`:

```json
{
  "device_id": "pv-monitoring-01",
  "firmware_version": "1.0.0",
  "wifi_ssid": "MyWiFi",
  "wifi_rssi": -58,
  "ip": "192.168.68.42",
  "uptime_ms": 123456,
  "status": "online"
}
```

### 3. Topic subs - command yang didukung

Kirim payload ke `pv-monitoring/<DeviceID>/subs`:

| Payload | Reaksi |
| --- | --- |
| `ping` atau `{"cmd":"ping"}` | Device langsung publish ulang payload status ke `pv-monitoring/<DeviceID>/status` |

Command yang tidak dikenal akan di-log ke Serial lalu diabaikan.

## Integrasi InfluxDB v2

Selain publish MQTT, pada tiap siklus `publishIntervalMs` firmware juga melakukan `POST /api/v2/write?org=<org>&bucket=<bucket>&precision=ms` ke InfluxDB v2 dengan body line protocol seperti berikut:

```
pv_monitoring,device_id=pv-monitoring-01,charging_status=0 mppt_pv_voltage=18.500,mppt_charging_power=120.000,mppt_charging_current=6.480,mppt_battery_voltage=13.800,mppt_load_current=1.250,mppt_load_power=17.300,mppt_fault_code=0i,inverter_valid=true,inverter_ac_voltage=220.400,inverter_ac_current=0.431,inverter_ac_power=92.500,inverter_ac_energy=1.234,inverter_ac_frequency=50.000,inverter_ac_power_factor=0.970,inverter_ac_apparent_power=95.400,wifi_rssi=-58i
```

Header yang dipakai:

- `Authorization: Token <INFLUX_TOKEN>`
- `Content-Type: text/plain; charset=utf-8`

InfluxDB v2 mengembalikan `204 No Content` jika tulis sukses. Untuk menonaktifkan InfluxDB tanpa menghapus kode, set `config.influxConfig.enabled = false` di `main.cpp`.

### Menyiapkan InfluxDB v2

1. Buka UI InfluxDB di `http://192.168.68.106:8086`, login.
2. Buat bucket, misalnya `solar`.
3. Buat API token dengan permission **Write** ke bucket tersebut.
4. Isi `kInfluxOrg`, `kInfluxBucket`, `kInfluxToken` di `src/main.cpp`.

## Cara Mengubah Konfigurasi Runtime

Semua konfigurasi di-inject lewat `SendDataTask::Config`. Contoh di `src/main.cpp`:

```cpp
#include <Arduino.h>
#include <SendDataTask.h>
#include <WifiTask.h>
#include <dataTask.h>

void setup() {
  Serial.begin(115200);
  delay(1000);

  WifiTask::begin();
  DataTask::begin();

  SendDataTask::Config sendConfig{};
  sendConfig.firmwareVersion = "1.0.0";
  sendConfig.publishIntervalMs = 10000;
  sendConfig.statusIntervalMs = 30000;

  sendConfig.mqttConfig.server = "192.168.1.10";
  sendConfig.mqttConfig.port = 1883;
  sendConfig.mqttConfig.deviceId = "pv-monitoring-01";
  sendConfig.mqttConfig.topicPrefix = "pv-monitoring";
  sendConfig.mqttConfig.username = "mqtt-user";
  sendConfig.mqttConfig.password = "mqtt-pass";

  sendConfig.influxConfig.url = "http://192.168.68.106:8086";
  sendConfig.influxConfig.org = "YOUR_ORG";
  sendConfig.influxConfig.bucket = "solar";
  sendConfig.influxConfig.token = "YOUR_INFLUXDB_V2_TOKEN";
  sendConfig.influxConfig.measurement = "pv_monitoring";
  sendConfig.influxConfig.enabled = true;

  SendDataTask::begin(sendConfig);
}

void loop() {
  delay(100);
}
```

## Alur Kerja

1. Device konek WiFi (WifiTask).
2. `SendDataTask` konek ke broker MQTT dan subscribe ke topic `pv-monitoring/<DeviceID>/subs`.
3. Setelah konek, firmware publish status awal ke `pv-monitoring/<DeviceID>/status`.
4. `DataTask` terus-menerus poll MPPT (Modbus RTU) dan inverter (PZEM-004Tv3).
5. Tiap `publishIntervalMs`, `SendDataTask` publish data JSON ke `.../pubs` dan menulis line protocol ke InfluxDB v2.
6. Tiap `statusIntervalMs`, firmware publish status device ke `.../status`.
7. Saat ada message di `.../subs`, firmware eksekusi command (saat ini: `ping`).

## Build

```powershell
pio run -e 4d_systems_esp32s3_gen4_r8n16
```

## Catatan

- Buffer MQTT diset `1024` agar payload JSON data tetap muat.
- Stack task `SendDataTask` dinaikkan menjadi 8192 karena HTTPClient + buffer line protocol.
- Di serial monitor, kirim karakter `R` atau `r` untuk reset counter energy inverter.
- InfluxDB v2 endpoint default (`kInfluxUrl`) adalah `http://192.168.68.106:8086` sesuai permintaan.

# PV Monitoring System

Firmware ESP32 untuk membaca data MPPT dan inverter, lalu mengirimkannya ke broker MQTT menggunakan `PubSubClient` dan ke **InfluxDB v2** memakai **`tobiasschuerg/ESP8266 Influxdb`** (`InfluxDBClient` + `Point` + `WriteOptions`).

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
- `tobiasschuerg/ESP8266 Influxdb` (`InfluxDbClient`, `Point`, `WriteOptions`; sinkron waktu NTP lewat `configTime` / `TZ`)

## Konfigurasi Default

Lihat `src/main.cpp` untuk konstanta yang perlu diisi sebelum flashing:

| Konstanta | Keterangan |
| --- | --- |
| `kDeviceId` | ID perangkat, dipakai di seluruh topic MQTT (`pv-monitoring/<kDeviceId>/...`) |
| `kFirmwareVersion` | Versi firmware, dikirim pada payload status |
| `kMqttServer` / `kMqttPort` | Alamat broker MQTT |
| `kMqttUsername` / `kMqttPassword` | Kredensial MQTT (boleh string kosong) |
| `kInfluxUrl` | Base URL InfluxDB v2, contoh `http://192.168.68.106:8086` |
| `kInfluxOrg` | Organisasi InfluxDB v2 — **nama** atau **org ID** (hex) dari UI |
| `kInfluxBucket` | Bucket tujuan |
| `kInfluxToken` | API token InfluxDB v2 dengan permission write ke bucket |
| `kInfluxMeasurement` | Nama measurement untuk `Point` (default `pv_monitoring`) |
| `kInfluxTzInfo` | String `TZ` POSIX untuk `setenv` + NTP (default `UTC7`, sama contoh Influx Arduino) |

## Daftar Topic MQTT

Firmware ini **bukan** integrasi Home Assistant. Hanya ada tiga topic yang semuanya dibentuk dari `pv-monitoring/<DeviceID>/...`.

Contoh dengan `DeviceID = pv-monitoring-01`:

| Topic | Arah | Fungsi |
| --- | --- | --- |
| `pv-monitoring/pv-monitoring-01/pubs` | Device -> Broker | Publish data sensor MPPT + inverter periodik (default 10 s) |
| `pv-monitoring/pv-monitoring-01/subs` | Broker -> Device | Device subscribe di topic ini untuk menerima command |
| `pv-monitoring/info` | Device -> Broker | **Satu topic global** untuk status semua device; bedakan dengan field `device_id` di JSON |

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

### 2. Topic `pv-monitoring/info` — status device (global)

Semua unit publish ke **topic yang sama** `pv-monitoring/info`. Subscriber memakai field **`device_id`** untuk memilah perangkat.

Contoh payload:

```json
{
  "device_id": "pv-monitoring-01",
  "firmware_version": "1.0.0",
  "wifi_ssid": "MyWiFi",
  "wifi_rssi": -58,
  "ip": "192.168.68.42",
  "uptime_ms": 123456,
  "mqtt": "connected",
  "pv_sensor_data_ok": false,
  "mppt_poll_status": "Timeout",
  "inverter_read_status": "Read Failed",
  "send_task_status": "Waiting PV Data",
  "data_topic": "pv-monitoring/pv-monitoring-01/pubs",
  "status": "online"
}
```

Field `pv_sensor_data_ok`, `mppt_poll_status`, `inverter_read_status`, dan `send_task_status` membantu saat **data tidak terkirim ke `pubs`** karena sensor tidak terhubung atau gagal baca.

### 3. Topic subs - command yang didukung

Kirim payload ke `pv-monitoring/<DeviceID>/subs`:

| Payload | Reaksi |
| --- | --- |
| `ping` atau `{"cmd":"ping"}` | Device langsung publish ulang payload ke **`pv-monitoring/info`** |

Command yang tidak dikenal akan di-log ke Serial lalu diabaikan.

## Integrasi InfluxDB v2

Selain publish MQTT, pada tiap siklus `publishIntervalMs` firmware membangun **`Point`** (measurement `pv_monitoring`, tag `device_id` + `charging_status`, field sama seperti JSON MPPT/inverter) dan mengirimnya lewat **`InfluxDBClient::writePoint()`** dari library **`tobiasschuerg/ESP8266 Influxdb`** (konstruktor 4 argumen: URL, org, bucket, token — koneksi HTTP ke server lokal).

Setelah WiFi terhubung, firmware melakukan **sinkron NTP** (`configTime` + `setenv("TZ", ...)`) sekali per siklus koneksi WiFi, lalu **`validateConnection()`** (contoh resmi Influx) untuk mengecek koneksi ke server.

Untuk menonaktifkan tulis Influx, set `kInfluxEnabled = false` di `src/main.cpp`.

### Menyiapkan InfluxDB v2

1. Buka UI InfluxDB di `http://192.168.68.106:8086`, login.
2. Buat bucket (contoh default firmware: `pv-monitoring`).
3. Buat API token dengan permission **Write** ke bucket tersebut.
4. Isi `kInfluxOrg` (nama atau **org ID** hex dari UI), `kInfluxBucket`, `kInfluxToken` di `src/main.cpp`.

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
  sendConfig.publishIntervalMs = 5000;
  sendConfig.statusIntervalMs = 30000;

  sendConfig.mqttConfig.server = "192.168.1.10";
  sendConfig.mqttConfig.port = 1883;
  sendConfig.mqttConfig.deviceId = "pv-monitoring-01";
  sendConfig.mqttConfig.topicPrefix = "pv-monitoring";
  sendConfig.mqttConfig.username = "mqtt-user";
  sendConfig.mqttConfig.password = "mqtt-pass";

  sendConfig.influx.url = "http://192.168.68.106:8086";
  sendConfig.influx.org = "YOUR_ORG_OR_ORG_ID";
  sendConfig.influx.bucket = "pv-monitoring";
  sendConfig.influx.token = "YOUR_INFLUXDB_V2_TOKEN";
  sendConfig.influx.measurement = "pv_monitoring";
  sendConfig.influx.tzInfo = "UTC7";
  sendConfig.influx.enabled = true;

  SendDataTask::begin(sendConfig);
}

void loop() {
  delay(100);
}
```

## Alur Kerja

1. Device konek WiFi (WifiTask).
2. `SendDataTask` konek ke broker MQTT dan subscribe ke topic `pv-monitoring/<DeviceID>/subs`.
3. Setelah konek, firmware publish info awal ke **`pv-monitoring/info`** (setelah status sensor/task diperbarui).
4. `DataTask` terus-menerus poll MPPT (Modbus RTU) dan inverter (PZEM-004Tv3).
5. Tiap `publishIntervalMs`, `SendDataTask` publish data JSON ke `.../pubs` dan menulis **`Point`** ke InfluxDB v2 lewat library Arduino.
6. Tiap `statusIntervalMs`, firmware publish info device ke **`pv-monitoring/info`**.
7. Saat ada message di `.../subs`, firmware eksekusi command (saat ini: `ping`).

## Build

```powershell
pio run -e 4d_systems_esp32s3_gen4_r8n16
```

## Catatan

- Buffer MQTT diset `1024` agar payload JSON data tetap muat.
- Stack task `SendDataTask` 8192 byte karena library Influx memakai HTTP client di bawahnya.
- Di serial monitor, kirim karakter `R` atau `r` untuk reset counter energy inverter.
- InfluxDB v2 endpoint default (`kInfluxUrl`) adalah `http://192.168.68.106:8086` sesuai permintaan.

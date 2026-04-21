# PV Monitoring System

Firmware ESP32 untuk membaca data MPPT dan inverter, lalu mengirimkannya ke broker MQTT menggunakan `PubSubClient`.

## Ringkasan

Sistem ini menjalankan tiga task utama:

1. `WifiTask` untuk koneksi WiFi
2. `DataTask` untuk membaca sensor MPPT dan inverter
3. `SendDataTask` untuk publish data ke MQTT

Board target saat ini ada di `platformio.ini`:

- Environment: `4d_systems_esp32s3_gen4_r8n16`
- Framework: `Arduino`

## Library yang dipakai

- `PubSubClient`
- `WiFiManager`
- `PZEM-004T-v30`

## Konfigurasi MQTT Default

Default MQTT saat ini didefinisikan di `lib/Modules/MQTTConfig/MQTTConfig.h`.

| Parameter | Default |
| --- | --- |
| `server` | `broker.hivemq.com` |
| `port` | `1883` |
| `clientId` | `pv-monitoring-client` |
| `username` | `""` |
| `password` | `""` |
| `dataTopic` | `pv-monitoring/data` |
| `infoTopic` | `pv-monitoring/info` |
| `bufferSize` | `1024` |
| `reconnectIntervalMs` | `5000` |

## Daftar Topic MQTT

Saat ini firmware hanya publish ke dua topic berikut:

### 1. Topic data

- Topic: `pv-monitoring/data`
- Arah: publish
- Fungsi: kirim data MPPT dan inverter secara periodik
- Interval default: `10000 ms`

Payload JSON yang dikirim:

```json
{
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

Keterangan field:

| Field | Tipe | Keterangan |
| --- | --- | --- |
| `mppt_valid` | `bool` | Penanda data MPPT valid saat publish |
| `mppt_pv_voltage` | `float` | Tegangan panel surya, satuan volt |
| `mppt_charging_power` | `float` | Daya charging MPPT, satuan watt |
| `mppt_charging_current` | `float` | Arus charging MPPT, satuan ampere |
| `mppt_battery_voltage` | `float` | Tegangan baterai, satuan volt |
| `mppt_charging_status` | `uint8` | Status charging numerik dari MPPT |
| `mppt_charging_status_text` | `string` | Status charging dalam teks |
| `mppt_load_current` | `float` | Arus beban DC, satuan ampere |
| `mppt_load_power` | `float` | Daya beban DC, satuan watt |
| `mppt_fault_code` | `uint16` | Fault code dari MPPT |
| `mppt_timestamp_ms` | `uint32` | Timestamp data MPPT berdasarkan `millis()` |
| `inverter_valid` | `bool` | Penanda data inverter valid atau tidak |
| `inverter_ac_voltage` | `float` | Tegangan AC inverter, satuan volt |
| `inverter_ac_current` | `float` | Arus AC inverter, satuan ampere |
| `inverter_ac_power` | `float` | Daya aktif inverter, satuan watt |
| `inverter_ac_energy` | `float` | Energi inverter, satuan kWh |
| `inverter_ac_frequency` | `float` | Frekuensi AC, satuan Hz |
| `inverter_ac_power_factor` | `float` | Power factor inverter |
| `inverter_ac_apparent_power` | `float` | Daya semu inverter, satuan VA |
| `inverter_timestamp_ms` | `uint32` | Timestamp data inverter berdasarkan `millis()` |
| `wifi_rssi` | `int32` | Kekuatan sinyal WiFi |

Nilai `mppt_charging_status_text` saat ini:

- `MPPT Charging`
- `Boost / Equalizing`
- `Floating`
- `Not Charging`
- `Unknown`

### 2. Topic info

- Topic: `pv-monitoring/info`
- Arah: publish
- Fungsi: kirim informasi koneksi MQTT saat perangkat berhasil terkoneksi
- Dikirim sekali setiap selesai konek MQTT jika `publishInfoOnConnect = true`

Payload JSON yang dikirim:

```json
{
  "client_id": "pv-monitoring-client",
  "broker": "broker.hivemq.com",
  "port": 1883,
  "data_topic": "pv-monitoring/data",
  "info_topic": "pv-monitoring/info"
}
```

Field pada topic info:

| Field | Tipe | Keterangan |
| --- | --- | --- |
| `client_id` | `string` | Client ID MQTT |
| `broker` | `string` | Alamat broker MQTT |
| `port` | `uint16` | Port broker MQTT |
| `data_topic` | `string` | Topic publish data sensor |
| `info_topic` | `string` | Topic publish informasi koneksi |

### 3. Topic subscribe

Saat ini firmware belum subscribe topic apa pun.

- Tidak ada topic command
- Tidak ada RPC
- Tidak ada control topic dari broker ke device

Kalau ingin menambah subscribe, entry point paling cocok ada di `MQTTConfigManager::client()`.

## Cara Mengubah Topic MQTT

Topic MQTT bisa diubah lewat `SendDataTask::Config` dan `MQTTModule::Config`.

Contoh penggunaan di `src/main.cpp`:

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

  MQTTModule::Config mqttConfig{};
  mqttConfig.server = "192.168.1.10";
  mqttConfig.port = 1883;
  mqttConfig.clientId = "esp32-pv-monitor";
  mqttConfig.username = "mqtt-user";
  mqttConfig.password = "mqtt-pass";
  mqttConfig.dataTopic = "solar/pv-monitor/data";
  mqttConfig.infoTopic = "solar/pv-monitor/info";

  SendDataTask::Config sendConfig{};
  sendConfig.mqttConfig = mqttConfig;
  sendConfig.publishIntervalMs = 10000;
  sendConfig.publishInfoOnConnect = true;

  SendDataTask::begin(sendConfig);
}

void loop() {
  delay(100);
}
```

## Alur Publish MQTT

Urutan kerjanya seperti ini:

1. Device konek WiFi
2. `SendDataTask` mencoba konek ke broker MQTT
3. Setelah konek, firmware publish payload info ke `infoTopic`
4. Firmware membaca data MPPT dan inverter
5. Firmware publish payload data ke `dataTopic` secara periodik

## Build

Command build:

```powershell
pio run -e 4d_systems_esp32s3_gen4_r8n16
```

## Catatan

- Payload data sudah dipisah jelas antara data MPPT dan inverter dengan prefix `mppt_` dan `inverter_`.
- Default koneksi MQTT sekarang adalah MQTT umum, bukan ThingsBoard.
- Buffer MQTT diset `1024` agar payload JSON data tetap muat.
- Di serial monitor, kirim karakter `R` atau `r` untuk reset counter energy inverter.

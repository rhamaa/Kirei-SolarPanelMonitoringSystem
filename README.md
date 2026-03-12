# Solar Panel Monitoring System (R&D)

ESP32-based solar panel monitoring prototype that publishes telemetry to ThingsBoard Cloud via MQTT.

## Overview

- ESP32 connects to WiFi using **WiFiManager** (no hardcoded SSID/password).
- Telemetry is published to ThingsBoard using **MQTT** on topic `v1/devices/me/telemetry`.
- Current firmware uses **randomized simulated telemetry** values for R&D.

## Requirements

### Hardware

- ESP32 (tested with Arduino framework)

### Software

- Arduino IDE (or compatible)
- Arduino libraries:
  - `WiFiManager` (tzapu)
  - `PubSubClient`

## ThingsBoard Setup

1. Create a device in ThingsBoard Cloud.
2. Open the device and copy the **Access token**.
3. Firmware uses:
   - MQTT host: `mqtt.thingsboard.cloud`
   - MQTT port: `1883` (non-TLS)
   - Telemetry topic: `v1/devices/me/telemetry`
   - MQTT username: **Device Access Token**

## Firmware Setup

Firmware entry point:

- `solar_panel_monitoring/solar_panel_monitoring.ino`

Update these constants in the sketch:

- `token` (Device Access Token)
- `client_id` (any unique string)

## WiFi Provisioning (WiFiManager)

1. Flash the firmware to ESP32.
2. If ESP32 has no saved WiFi (or cannot connect), it will create an AP:

- SSID: `SolarPanel-Setup`

3. Connect your phone/laptop to that AP.
4. Open the captive portal (usually auto). If not, open:

- `http://192.168.4.1`

5. Select your WiFi SSID and enter password.
6. ESP32 will reboot/connect and start publishing telemetry.

## Telemetry Keys

The firmware publishes these keys:

- `solarPower` (W)
- `voltage` (V)
- `amper` (A)
- `batteryStatus` (%)
- `outputPower` (W)

## Verify Telemetry

- In ThingsBoard: open the device and check **Latest telemetry**.
- In Serial Monitor (115200): you should see connect logs and `Data sent: {...}`.

## Troubleshooting

- **Device still inactive**: usually means telemetry is not arriving; verify device token and network.
- **MQTT not authorized**: token is wrong or not the device Access Token.
- **Connection lost**: network issues or blocked port; try a different network.

## Notes

- This repository is for **R&D**. Values are simulated and should be replaced with real sensor readings.
- Do not commit real credentials/tokens.

"""
Solar Panel MQTT Simulator + Battery + Dummy Load 1
Auto Discovery - semua dari satu device

Install: pip install paho-mqtt
Jalankan: python3 simulator.py
"""

import json, math, time, random
from datetime import datetime
import paho.mqtt.client as mqtt

# ============================================================
# KONFIGURASI
# ============================================================
MQTT_HOST      = "192.168.68.106"
MQTT_PORT      = 1883
MQTT_USER      = "mqtt_user"
MQTT_PASS      = "mqtt_password"
MQTT_CLIENT_ID = "solar-panel-01"
PUBLISH_INTERVAL = 5

DEVICE_ID      = "dummy_solar_panel_01"
DEVICE_NAME    = "Dummy Solar Panel 01"
STATE_TOPIC    = f"solar/{DEVICE_ID}/state"
AVAIL_TOPIC    = f"solar/{DEVICE_ID}/availability"
DISCOVERY_BASE = "homeassistant"

# ============================================================
# Definisi semua sensor
# ============================================================
SENSORS = [
    # --- Panel surya ---
    {
        "id": "pv_power", "name": "PV Power",
        "unit": "W", "device_class": "power",
        "state_class": "measurement", "icon": "mdi:solar-power",
        "value_key": "pv_power",
    },
    {
        "id": "pv_voltage", "name": "PV Voltage",
        "unit": "V", "device_class": "voltage",
        "state_class": "measurement", "icon": "mdi:flash",
        "value_key": "pv_voltage",
    },
    {
        "id": "pv_current", "name": "PV Current",
        "unit": "A", "device_class": "current",
        "state_class": "measurement", "icon": "mdi:current-dc",
        "value_key": "pv_current",
    },
    {
        "id": "energy_today", "name": "Energy Today",
        "unit": "kWh", "device_class": "energy",
        "state_class": "total_increasing", "icon": "mdi:lightning-bolt",
        "value_key": "energy_today",
    },

    # --- Baterai ---
    {
        "id": "battery_soc", "name": "Battery SOC",
        "unit": "%", "device_class": "battery",
        "state_class": "measurement", "icon": "mdi:battery",
        "value_key": "batt_soc",
    },
    {
        "id": "battery_voltage", "name": "Battery Voltage",
        "unit": "V", "device_class": "voltage",
        "state_class": "measurement", "icon": "mdi:battery-charging",
        "value_key": "batt_voltage",
    },
    {
        "id": "battery_energy_in", "name": "Battery Energy In",
        "unit": "kWh", "device_class": "energy",
        "state_class": "total_increasing", "icon": "mdi:battery-arrow-up",
        "value_key": "batt_energy_in",
    },
    {
        "id": "battery_energy_out", "name": "Battery Energy Out",
        "unit": "kWh", "device_class": "energy",
        "state_class": "total_increasing", "icon": "mdi:battery-arrow-down",
        "value_key": "batt_energy_out",
    },

    # --- Dummy Load 1 ---
    {
        "id": "load1_power", "name": "Dummy Load 1 Power",
        "unit": "W", "device_class": "power",
        "state_class": "measurement", "icon": "mdi:power-plug",
        "value_key": "load1_power",
    },
    {
        "id": "load1_energy", "name": "Dummy Load 1 Energy",
        "unit": "kWh", "device_class": "energy",
        "state_class": "total_increasing", "icon": "mdi:power-plug",
        "value_key": "load1_energy",
    },
]

# ============================================================
# Publish MQTT Discovery
# ============================================================
def publish_discovery(client):
    device_info = {
        "identifiers":  [DEVICE_ID],
        "name":         DEVICE_NAME,
        "model":        "MPPT Solar Monitor",
        "manufacturer": "DIY",
        "sw_version":   "2.0.0",
    }
    for sensor in SENSORS:
        topic = f"{DISCOVERY_BASE}/sensor/{DEVICE_ID}/{sensor['id']}/config"
        payload = {
            "name":                sensor["name"],
            "unique_id":           f"{DEVICE_ID}_{sensor['id']}",
            "state_topic":         STATE_TOPIC,
            "availability_topic":  AVAIL_TOPIC,
            "value_template":      f"{{{{ value_json.{sensor['value_key']} }}}}",
            "unit_of_measurement": sensor["unit"],
            "device_class":        sensor["device_class"],
            "state_class":         sensor["state_class"],
            "icon":                sensor["icon"],
            "device":              device_info,
        }
        client.publish(topic, json.dumps(payload), retain=True)
        print(f"  [Discovery] Registered: {sensor['name']}")
    print(f"[Discovery] Semua {len(SENSORS)} sensor terdaftar!\n")

# ============================================================
# Simulator
# ============================================================
class SolarSimulator:
    def __init__(self):
        self.energy_today    = 0.0
        self.batt_energy_in  = 0.0
        self.batt_energy_out = 0.0
        self.load1_energy    = 0.0
        self.last_time       = time.time()
        self.batt_soc        = 60.0

    def generate(self):
        now = time.time()
        dt  = now - self.last_time
        self.last_time = now

        # Faktor sinar matahari berdasarkan jam
        hour   = datetime.now().hour + datetime.now().minute / 60.0
        factor = max(0.0, math.sin(math.pi * (hour - 6) / 12)) if 6 <= hour <= 18 else 0.0
        factor = max(0.0, min(1.0, factor + random.gauss(0, 0.05)))

        # Panel surya (400Wp)
        pv_power   = round(max(0.0, 400.0 * factor + random.gauss(0, 3)), 2)
        pv_voltage = round(22.0 + factor * 6.0 + random.gauss(0, 0.3), 2)
        pv_current = round(pv_power / pv_voltage, 3) if pv_voltage > 0 else 0.0

        # Dummy Load 1 (30W konstan dengan sedikit fluktuasi)
        load1_power = round(max(0.0, 30.0 + random.gauss(0, 1.5)), 2)

        # Net power: surplus masuk baterai, defisit keluar baterai
        net_power = pv_power - load1_power
        if net_power > 0:
            self.batt_energy_in  += (net_power * dt) / 3_600_000.0
        else:
            self.batt_energy_out += (abs(net_power) * dt) / 3_600_000.0

        # Update SOC (baterai 12V 100Ah = 1200Wh)
        soc_delta = (net_power * dt) / (1200.0 * 3600.0) * 100.0
        self.batt_soc = max(5.0, min(100.0, self.batt_soc + soc_delta))
        batt_voltage  = round(11.5 + (self.batt_soc / 100) * 2.5 + random.gauss(0, 0.05), 2)

        # Akumulasi energi
        self.energy_today += (pv_power    * dt) / 3_600_000.0
        self.load1_energy += (load1_power * dt) / 3_600_000.0

        return {
            "pv_voltage":      pv_voltage,
            "pv_current":      pv_current,
            "pv_power":        pv_power,
            "energy_today":    round(self.energy_today, 4),
            "batt_soc":        round(self.batt_soc, 1),
            "batt_voltage":    batt_voltage,
            "batt_energy_in":  round(self.batt_energy_in, 4),
            "batt_energy_out": round(self.batt_energy_out, 4),
            "load1_power":     load1_power,
            "load1_energy":    round(self.load1_energy, 4),
        }

# ============================================================
# MQTT callbacks
# ============================================================
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("[MQTT] Terhubung ke broker!")
        client.publish(AVAIL_TOPIC, "online", retain=True)
        print("[Discovery] Mendaftarkan sensor...")
        publish_discovery(client)
    else:
        print(f"[MQTT] Gagal connect rc={rc}")

def on_disconnect(client, userdata, rc):
    if rc != 0:
        print(f"[MQTT] Terputus rc={rc}")

# ============================================================
# Main
# ============================================================
def main():
    print("=" * 60)
    print("  Solar Simulator v2.0  |  Battery + Dummy Load 1")
    print(f"  Broker  : {MQTT_HOST}:{MQTT_PORT}")
    print(f"  Device  : {DEVICE_NAME}")
    print(f"  Interval: {PUBLISH_INTERVAL}s")
    print("=" * 60 + "\n")

    sim    = SolarSimulator()
    client = mqtt.Client(client_id=MQTT_CLIENT_ID)
    client.username_pw_set(MQTT_USER, MQTT_PASS)
    client.will_set(AVAIL_TOPIC, "offline", retain=True)
    client.on_connect    = on_connect
    client.on_disconnect = on_disconnect

    print(f"[MQTT] Menghubungkan ke {MQTT_HOST}:{MQTT_PORT}...")
    try:
        client.connect(MQTT_HOST, MQTT_PORT, keepalive=60)
    except Exception as e:
        print(f"[ERROR] {e}")
        return

    client.loop_start()
    time.sleep(2)

    print("Mengirim data... Ctrl+C untuk berhenti\n")
    print(f"{'Waktu':<10} {'PV':>8} {'Batt V':>8} {'SOC':>6} {'Load1':>7} {'E.Solar':>9} {'B.In':>9} {'B.Out':>9}")
    print("-" * 72)

    try:
        while True:
            data = sim.generate()
            client.publish(STATE_TOPIC, json.dumps(data), retain=True)

            t = datetime.now().strftime("%H:%M:%S")
            print(
                f"{t:<10}"
                f"{data['pv_power']:>7.1f}W"
                f"{data['batt_voltage']:>7.2f}V"
                f"{data['batt_soc']:>5.1f}%"
                f"{data['load1_power']:>6.1f}W"
                f"{data['energy_today']:>8.4f}k"
                f"{data['batt_energy_in']:>8.4f}k"
                f"{data['batt_energy_out']:>8.4f}k"
            )
            time.sleep(PUBLISH_INTERVAL)

    except KeyboardInterrupt:
        print("\n[Simulator] Berhenti.")
        client.publish(AVAIL_TOPIC, "offline", retain=True)
        time.sleep(0.5)
        client.loop_stop()
        client.disconnect()

if __name__ == "__main__":
    main()
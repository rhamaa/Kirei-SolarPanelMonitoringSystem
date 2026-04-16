import json, math, time, random
from datetime import datetime
import paho.mqtt.client as mqtt

MQTT_HOST      = "192.168.68.106"
MQTT_PORT      = 1883
MQTT_USER      = "mqtt_user"
MQTT_PASS      = "mqtt_password"
MQTT_CLIENT_ID = "solar-simulator-py"
PUBLISH_INTERVAL = 5

TOPICS = {
    "power":        "solar/panel/power",
    "voltage":      "solar/panel/voltage",
    "current":      "solar/panel/current",
    "batt_voltage": "solar/battery/voltage",
    "batt_soc":     "solar/battery/soc",
    "energy_today": "solar/panel/energy_today",
    "status":       "solar/panel/status",
    "state":        "solar/panel/state",
}

class SolarSimulator:
    def __init__(self):
        self.energy = 0.0
        self.last_time = time.time()
        self.batt_soc = 60.0

    def generate(self):
        now = time.time()
        dt = now - self.last_time
        self.last_time = now
        hour = datetime.now().hour + datetime.now().minute / 60.0
        factor = max(0.0, math.sin(math.pi * (hour - 6) / 12)) if 6 <= hour <= 18 else 0.0
        factor = max(0.0, min(1.0, factor + random.gauss(0, 0.05)))

        pv_power    = round(max(0.0, 400.0 * factor + random.gauss(0, 3)), 2)
        pv_voltage  = round(22.0 + factor * 6.0 + random.gauss(0, 0.3), 2)
        pv_current  = round(pv_power / pv_voltage, 3) if pv_voltage > 0 else 0.0
        batt_voltage = round(11.5 + (self.batt_soc / 100) * 2.5 + random.gauss(0, 0.05), 2)

        net = pv_power - 30.0
        self.batt_soc = max(5.0, min(100.0, self.batt_soc + (net * dt) / (12 * 100 * 3600) * 100))
        self.energy += (pv_power * dt) / 3600000.0

        return {
            "pv_voltage": pv_voltage, "pv_current": pv_current,
            "pv_power": pv_power, "batt_voltage": batt_voltage,
            "batt_soc": round(self.batt_soc, 1),
            "energy_today": round(self.energy, 3),
        }

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("[MQTT] Terhubung ke broker!")
        client.publish(TOPICS["status"], "online", retain=True)
    else:
        print(f"[MQTT] Gagal connect, rc={rc}")

sim = SolarSimulator()
client = mqtt.Client(client_id=MQTT_CLIENT_ID)
client.username_pw_set(MQTT_USER, MQTT_PASS)
client.will_set(TOPICS["status"], "offline", retain=True)
client.on_connect = on_connect
client.connect(MQTT_HOST, MQTT_PORT, 60)
client.loop_start()
time.sleep(1)

print("Simulator berjalan... Ctrl+C untuk berhenti\n")
try:
    while True:
        d = sim.generate()
        for key in ["voltage","current","power","batt_voltage","batt_soc","energy_today"]:
            topic_key = key if key in TOPICS else key
            val = d.get(f"pv_{key}", d.get(key, d.get(f"batt_{key.replace('batt_','')}", "")))
        client.publish(TOPICS["voltage"],      str(d["pv_voltage"]),   retain=True)
        client.publish(TOPICS["current"],      str(d["pv_current"]),   retain=True)
        client.publish(TOPICS["power"],        str(d["pv_power"]),     retain=True)
        client.publish(TOPICS["batt_voltage"], str(d["batt_voltage"]), retain=True)
        client.publish(TOPICS["batt_soc"],     str(d["batt_soc"]),     retain=True)
        client.publish(TOPICS["energy_today"], str(d["energy_today"]), retain=True)
        client.publish(TOPICS["state"],        json.dumps(d),          retain=True)
        t = datetime.now().strftime("%H:%M:%S")
        print(f"[{t}] PV: {d['pv_power']:6.1f}W  {d['pv_voltage']:5.2f}V  {d['pv_current']:5.3f}A  |  Batt: {d['batt_voltage']:5.2f}V  SOC:{d['batt_soc']:5.1f}%  |  E:{d['energy_today']:.3f}kWh")
        time.sleep(PUBLISH_INTERVAL)
except KeyboardInterrupt:
    print("\nBerhenti.")
    client.publish(TOPICS["status"], "offline", retain=True)
    time.sleep(0.5)
    client.disconnect()
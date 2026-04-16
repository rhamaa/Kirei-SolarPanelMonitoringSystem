import paho.mqtt.client as mqtt
import json
import random
import time
from datetime import datetime

# ThingsBoard Cloud MQTT broker configuration
MQTT_BROKER = "mqtt.thingsboard.cloud"
MQTT_PORT = 1883
DEVICE_TOKEN = "ieR5Bsvd21oWBEQP9dhs"  # Device Access Token dari ThingsBoard
CLIENT_ID = "solar_panel_rnd_python"

# MQTT topic untuk telemetry
TELEMETRY_TOPIC = "v1/devices/me/telemetry"

# Global MQTT client
mqtt_client = None
is_connected = False


def on_connect(client, userdata, flags, rc):
    """Callback ketika connect ke MQTT broker"""
    global is_connected
    if rc == 0:
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] Connected to ThingsBoard Cloud")
        is_connected = True
    else:
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] Connection failed with code {rc}")
        is_connected = False


def on_disconnect(client, userdata, rc):
    """Callback ketika disconnect dari MQTT broker"""
    global is_connected
    if rc != 0:
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] Unexpected disconnection: {rc}")
    is_connected = False


def on_publish(client, userdata, mid):
    """Callback ketika publish berhasil"""
    print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] Message published successfully")


def on_log(client, userdata, level, buf):
    """Callback untuk logging MQTT"""
    if level == mqtt.MQTT_LOG_ERR:
        print(f"[MQTT ERROR] {buf}")


def setup_mqtt():
    """Setup dan connect ke MQTT broker"""
    global mqtt_client
    
    mqtt_client = mqtt.Client(client_id=CLIENT_ID)
    mqtt_client.on_connect = on_connect
    mqtt_client.on_disconnect = on_disconnect
    mqtt_client.on_publish = on_publish
    mqtt_client.on_log = on_log
    
    # Set username sebagai device token (ThingsBoard format)
    mqtt_client.username_pw_set(DEVICE_TOKEN, None)
    
    try:
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] Connecting to {MQTT_BROKER}:{MQTT_PORT}...")
        mqtt_client.connect(MQTT_BROKER, MQTT_PORT, keepalive=60)
        mqtt_client.loop_start()
        
        # Wait untuk connect
        timeout = 10
        start_time = time.time()
        while not is_connected and (time.time() - start_time) < timeout:
            time.sleep(0.1)
        
        if not is_connected:
            print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] Failed to connect within timeout")
            return False
        
        return True
    except Exception as e:
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] Connection error: {e}")
        return False


def generate_sensor_data():
    """Generate dummy sensor data (MPPT structure sesuai screenshot)"""
    data = {
        "pv_voltage": round(random.uniform(12, 50), 2),           # V -- PV panel voltage
        "pv_rated_v": round(random.uniform(100, 200), 1),         # V -- rated naik siang hari
        "battery_voltage": round(random.uniform(12, 60), 2),      # V -- battery voltage
        "battery_soc": random.randint(0, 100),                    # % -- battery state of charge
        "charging_power": round(random.uniform(0, 500), 1),       # W -- naik siang hari
        "charging_current": round(random.uniform(0, 50), 2),      # A -- dihitung W/V
        "load_current": round(random.uniform(0, 30), 2),          # A -- dihitung A*V
        "load_power": round(random.uniform(0, 500), 1),           # W -- dihitung A*V
        "ctrl_temp": round(random.uniform(20, 60), 1),            # C -- controller temperature
        "float_limit_v": round(random.uniform(13, 14.5), 2),      # V -- float limit voltage
        "charging_status": random.randint(0, 3),                  # 0=MPPT, 1=Boost, 2=Float, 3=Off
        "fault_code": 0                                            # bitmask idx28 0x101C
    }
    return data


def publish_telemetry(data):
    """Publish telemetry ke ThingsBoard"""
    if not is_connected:
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] Not connected to MQTT broker")
        return False
    
    try:
        payload = json.dumps(data)
        result = mqtt_client.publish(TELEMETRY_TOPIC, payload, qos=1)
        
        if result.rc == mqtt.MQTT_ERR_SUCCESS:
            print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] Data sent: {payload}")
            return True
        else:
            print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] Publish failed with code {result.rc}")
            return False
    except Exception as e:
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] Publish error: {e}")
        return False


def main():
    """Main loop - simulasi sensor dan publish ke ThingsBoard"""
    print("=" * 60)
    print("Solar Panel Monitoring - ThingsBoard MQTT (Python)")
    print("=" * 60)
    
    # Setup MQTT connection
    if not setup_mqtt():
        print("Failed to setup MQTT connection. Exiting.")
        return
    
    print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] Starting telemetry loop (5 second interval)...")
    print("=" * 60)
    
    try:
        while True:
            # Generate dummy sensor data
            sensor_data = generate_sensor_data()
            
            # Publish ke ThingsBoard
            publish_telemetry(sensor_data)
            
            # Wait 5 seconds sebelum publish berikutnya
            time.sleep(5)
    
    except KeyboardInterrupt:
        print(f"\n[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] Shutting down...")
    except Exception as e:
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] Unexpected error: {e}")
    finally:
        if mqtt_client:
            mqtt_client.loop_stop()
            mqtt_client.disconnect()
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] Disconnected from ThingsBoard")


if __name__ == "__main__":
    main()

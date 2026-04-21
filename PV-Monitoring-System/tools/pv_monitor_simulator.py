#!/usr/bin/env python3
"""
Dummy simulator for PV-Monitoring-System firmware:
- Publishes JSON to MQTT `.../pubs` and global `pv-monitoring/info` (same idea as ESP32).
- Writes to InfluxDB v2 using the official **influxdb-client** Python library (`Point` + `write_api`).

Usage:
  cd tools
  pip install -r requirements-simulator.txt

  # Wajib untuk Influx (tanpa ini, Influx dilewati sama sekali):
  set INFLUX_TOKEN=...   # PowerShell: $env:INFLUX_TOKEN="..."

  python pv_monitor_simulator.py

Optional env vars:
  MQTT_HOST, MQTT_PORT, MQTT_USER, MQTT_PASSWORD
  DEVICE_ID, TOPIC_PREFIX
  INFLUX_URL, INFLUX_ORG, INFLUX_BUCKET, INFLUX_MEASUREMENT, INFLUX_TOKEN
  INTERVAL_SEC, STATUS_INTERVAL_SEC, SIM_CLIENT_ID
  INFLUX_VERIFY=1  — cek bucket terlihat dari API + satu query tes setelah write pertama
"""

from __future__ import annotations

import json
import math
import os
import sys
import time
from dataclasses import dataclass
from typing import Any

try:
    from influxdb_client import InfluxDBClient, Point
    from influxdb_client.client.exceptions import InfluxDBError
    from influxdb_client.client.write_api import SYNCHRONOUS
    from influxdb_client.rest import ApiException
except ImportError:
    print("Install dependencies: pip install -r requirements-simulator.txt", file=sys.stderr)
    raise

try:
    import paho.mqtt.client as mqtt
except ImportError:
    print("Install dependencies: pip install -r requirements-simulator.txt", file=sys.stderr)
    raise


CHARGING_STATUS_TEXT = {
    0: "MPPT Charging",
    1: "Boost / Equalizing",
    2: "Floating",
    3: "Not Charging",
}


def _strip(s: str | None) -> str:
    return (s or "").strip()


@dataclass
class Config:
    mqtt_host: str
    mqtt_port: int
    mqtt_user: str
    mqtt_password: str
    device_id: str
    topic_prefix: str
    sim_client_id: str
    influx_url: str
    influx_org: str
    influx_bucket: str
    influx_measurement: str
    influx_token: str
    interval_sec: float
    status_interval_sec: float
    influx_verify: bool


def load_config() -> Config:
    return Config(
        mqtt_host=_strip(os.environ.get("MQTT_HOST", "192.168.68.106")) or "192.168.68.106",
        mqtt_port=int(os.environ.get("MQTT_PORT", "1883")),
        mqtt_user=_strip(os.environ.get("MQTT_USER", "mqtt_user")),
        mqtt_password=_strip(os.environ.get("MQTT_PASSWORD", "mqtt_password")),
        device_id=_strip(os.environ.get("DEVICE_ID", "pv-monitoring-01")) or "pv-monitoring-01",
        topic_prefix=_strip(os.environ.get("TOPIC_PREFIX", "pv-monitoring")) or "pv-monitoring",
        sim_client_id=_strip(os.environ.get("SIM_CLIENT_ID", "pv-sim-python")) or "pv-sim-python",
        influx_url=_strip(os.environ.get("INFLUX_URL", "http://192.168.68.106:8086")).rstrip("/")
        or "http://192.168.68.106:8086",
        influx_org=_strip(os.environ.get("INFLUX_ORG", "86d3a746830ba285")),
        influx_bucket=_strip(os.environ.get("INFLUX_BUCKET", "pv-monitoring")) or "pv-monitoring",
        influx_measurement=_strip(os.environ.get("INFLUX_MEASUREMENT", "pv_monitoring"))
        or "pv_monitoring",
        influx_token=_strip(os.environ.get("INFLUX_TOKEN", "")),
        interval_sec=float(os.environ.get("INTERVAL_SEC", "10")),
        status_interval_sec=float(os.environ.get("STATUS_INTERVAL_SEC", "30")),
        influx_verify=os.environ.get("INFLUX_VERIFY", "").strip() in ("1", "true", "yes"),
    )


def topic_pubs(cfg: Config) -> str:
    return f"{cfg.topic_prefix}/{cfg.device_id}/pubs"


def topic_device_info(cfg: Config) -> str:
    """Satu topic global untuk semua device (sama dengan firmware ESP32)."""
    return f"{cfg.topic_prefix}/info"


def topic_subs(cfg: Config) -> str:
    return f"{cfg.topic_prefix}/{cfg.device_id}/subs"


def fake_sensor_snapshot(t: float, cfg: Config) -> dict[str, Any]:
    """Values oscillate slightly to look alive on dashboards."""
    pv = 18.0 + 2.0 * math.sin(t / 40.0)
    batt = 13.2 + 0.4 * math.sin(t / 55.0)
    ichg = 4.0 + 1.5 * math.sin(t / 30.0)
    pchg = max(0.0, pv * ichg * 0.85)
    load_i = 0.8 + 0.3 * math.sin(t / 22.0)
    load_p = load_i * batt
    charging_status = int(t // 120.0) % 4
    ts_ms = int(time.time() * 1000) % (2**32)

    inv_v = 219.0 + 3.0 * math.sin(t / 18.0)
    inv_i = 0.35 + 0.1 * math.sin(t / 25.0)
    inv_p = max(0.0, inv_v * inv_i * 0.95)
    inv_e = (t / 3600.0) * 0.01  # fake kWh drift
    inv_hz = 50.0
    inv_pf = 0.96
    inv_s = inv_p / inv_pf if inv_pf > 0 else 0.0

    return {
        "device_id": cfg.device_id,
        "mppt_valid": True,
        "mppt_pv_voltage": round(pv, 1),
        "mppt_charging_power": round(pchg, 1),
        "mppt_charging_current": round(ichg, 2),
        "mppt_battery_voltage": round(batt, 1),
        "mppt_charging_status": charging_status,
        "mppt_charging_status_text": CHARGING_STATUS_TEXT.get(charging_status, "Unknown"),
        "mppt_load_current": round(load_i, 2),
        "mppt_load_power": round(load_p, 1),
        "mppt_fault_code": 0,
        "mppt_timestamp_ms": ts_ms,
        "inverter_valid": True,
        "inverter_ac_voltage": round(inv_v, 1),
        "inverter_ac_current": round(inv_i, 3),
        "inverter_ac_power": round(inv_p, 1),
        "inverter_ac_energy": round(inv_e, 3),
        "inverter_ac_frequency": round(inv_hz, 1),
        "inverter_ac_power_factor": round(inv_pf, 2),
        "inverter_ac_apparent_power": round(inv_s, 1),
        "inverter_timestamp_ms": ts_ms,
        "wifi_rssi": -55 + int(3 * math.sin(t / 12.0)),
    }


def status_payload(cfg: Config, start: float) -> dict[str, Any]:
    """Payload mirip firmware: `device_id` membedakan perangkat di topic global."""
    uptime_ms = int((time.time() - start) * 1000)
    pubs = topic_pubs(cfg)
    return {
        "device_id": cfg.device_id,
        "firmware_version": "sim-python-1.0",
        "wifi_ssid": "simulated",
        "wifi_rssi": -50,
        "ip": "192.168.68.199",
        "uptime_ms": uptime_ms,
        "mqtt": "connected",
        "pv_sensor_data_ok": True,
        "mppt_poll_status": "OK",
        "inverter_read_status": "OK",
        "send_task_status": "Ready",
        "data_topic": pubs,
        "status": "online",
    }


def snapshot_to_point(snapshot: dict[str, Any], cfg: Config) -> Point:
    """Same measurement/tags/fields as ESP32 `buildInfluxPoint` (InfluxDb Arduino client)."""
    cs = int(snapshot["mppt_charging_status"])
    inv_ok = bool(snapshot["inverter_valid"])

    def fv(key: str) -> float:
        return float(snapshot[key])

    # Pakai int untuk inverter_valid — beberapa pipeline lebih aman daripada bool di line protocol.
    p = (
        Point(cfg.influx_measurement)
        .tag("device_id", str(snapshot["device_id"]))
        .tag("charging_status", str(cs))
        .field("mppt_pv_voltage", fv("mppt_pv_voltage"))
        .field("mppt_charging_power", fv("mppt_charging_power"))
        .field("mppt_charging_current", fv("mppt_charging_current"))
        .field("mppt_battery_voltage", fv("mppt_battery_voltage"))
        .field("mppt_load_current", fv("mppt_load_current"))
        .field("mppt_load_power", fv("mppt_load_power"))
        .field("mppt_fault_code", int(snapshot["mppt_fault_code"]))
        .field("inverter_valid", 1 if inv_ok else 0)
        .field("inverter_ac_voltage", fv("inverter_ac_voltage"))
        .field("inverter_ac_current", fv("inverter_ac_current"))
        .field("inverter_ac_power", fv("inverter_ac_power"))
        .field("inverter_ac_energy", fv("inverter_ac_energy"))
        .field("inverter_ac_frequency", fv("inverter_ac_frequency"))
        .field("inverter_ac_power_factor", fv("inverter_ac_power_factor"))
        .field("inverter_ac_apparent_power", fv("inverter_ac_apparent_power"))
        .field("wifi_rssi", int(snapshot["wifi_rssi"]))
    )
    # Biarkan server isi waktu (hindari skew jam lokal vs Explorer "time range").
    return p


def influx_write_point(write_api: Any, cfg: Config, point: Point) -> tuple[bool, str]:
    try:
        write_api.write(bucket=cfg.influx_bucket, org=cfg.influx_org, record=point)
        return True, ""
    except ApiException as exc:
        body = (exc.body or "").strip()[:500]
        return False, f"ApiException HTTP {exc.status}: {body or exc.reason}"
    except InfluxDBError as exc:
        return False, f"InfluxDBError: {exc}"
    except Exception as exc:  # noqa: BLE001
        return False, f"{type(exc).__name__}: {exc}"[:400]


def verify_influx_setup(client: InfluxDBClient, cfg: Config) -> None:
    """Cek token/org/bucket agar error terlihat sebelum loop."""
    try:
        health = client.health()
        print(f"  Influx server health: {health.status} {health.message or ''}".rstrip())
    except Exception as exc:  # noqa: BLE001
        print(f"  Influx health() gagal (abaikan jika OSS lama): {exc}")

    try:
        buckets_api = client.buckets_api()
        names = [b.name for b in (buckets_api.find_buckets().buckets or [])]
        print(f"  Influx buckets terlihat dari token ({len(names)}): {names[:15]}{'...' if len(names) > 15 else ''}")
        if cfg.influx_bucket not in names:
            print(
                f"  *** PERINGATAN: bucket '{cfg.influx_bucket}' tidak ada di daftar. "
                f"Tulis akan gagal atau masuk ke org lain. Set INFLUX_BUCKET / INFLUX_ORG. ***"
            )
    except ApiException as exc:
        print(f"  Tidak bisa list buckets (izin token?): HTTP {exc.status} {exc.body or exc.reason}")
    except Exception as exc:  # noqa: BLE001
        print(f"  List buckets gagal: {exc}")


def flux_smoke_query(client: InfluxDBClient, cfg: Config) -> None:
    q = f'''
from(bucket: "{cfg.influx_bucket}")
  |> range(start: -24h)
  |> filter(fn: (r) => r["_measurement"] == "{cfg.influx_measurement}")
  |> limit(n: 3)
'''
    try:
        tables = client.query_api().query(q, org=cfg.influx_org)
        rows = 0
        for t in tables:
            for _ in t.records:
                rows += 1
        print(f"  Influx smoke query (24h, measurement={cfg.influx_measurement}): {rows} baris contoh")
    except Exception as exc:  # noqa: BLE001
        print(f"  Smoke query gagal: {exc}")


def main() -> None:
    cfg = load_config()
    pubs = topic_pubs(cfg)
    info_t = topic_device_info(cfg)
    subs = topic_subs(cfg)

    print("PV monitor simulator")
    print(f"  MQTT {cfg.mqtt_host}:{cfg.mqtt_port} client_id={cfg.sim_client_id}")
    print(f"  publish -> {pubs}")
    print(f"  publish -> {info_t}")
    print(f"  subscribe (optional log) <- {subs}")

    influx_client: InfluxDBClient | None = None
    write_api: Any = None
    if not cfg.influx_token:
        print("")
        print("  *** INFLUX_TOKEN kosong — tidak ada write ke Influx. ***")
        print("      PowerShell:  $env:INFLUX_TOKEN = 'token-dari-ui-influx'")
        print("      CMD:         set INFLUX_TOKEN=token-dari-ui-influx")
        print("")
    else:
        influx_client = InfluxDBClient(
            url=cfg.influx_url,
            token=cfg.influx_token,
            org=cfg.influx_org,
            timeout=30_000,
        )
        write_api = influx_client.write_api(write_options=SYNCHRONOUS)
        print(
            f"  Influx: url={cfg.influx_url} org={cfg.influx_org!r} bucket={cfg.influx_bucket!r} "
            f"measurement={cfg.influx_measurement!r}"
        )
        if cfg.influx_verify:
            verify_influx_setup(influx_client, cfg)

    start = time.time()
    last_status = 0.0
    first_influx_done = False

    def on_message(_client: mqtt.Client, _userdata: Any, msg: mqtt.MQTTMessage) -> None:
        try:
            body = msg.payload.decode("utf-8", errors="replace")
        except Exception:
            body = str(msg.payload)
        print(f"[MQTT RX] {msg.topic} <- {body!r}")

    try:
        client = mqtt.Client(  # type: ignore[attr-defined]
            mqtt.CallbackAPIVersion.VERSION2,
            client_id=cfg.sim_client_id,
        )
    except AttributeError:
        # paho-mqtt 1.x
        client = mqtt.Client(client_id=cfg.sim_client_id)
    client.username_pw_set(cfg.mqtt_user, cfg.mqtt_password)
    client.on_message = on_message

    client.connect(cfg.mqtt_host, cfg.mqtt_port, keepalive=60)
    client.subscribe(subs, qos=0)
    client.loop_start()

    try:
        while True:
            t = time.time() - start
            snap = fake_sensor_snapshot(t, cfg)
            payload = json.dumps(snap, separators=(",", ":"))

            info = client.publish(pubs, payload, qos=0, retain=False)
            info.wait_for_publish(timeout=5)
            print(f"[MQTT TX] {pubs} len={len(payload)} rc={info.rc}")

            if write_api is not None:
                point = snapshot_to_point(snap, cfg)
                ok, err = influx_write_point(write_api, cfg, point)
                tag = "OK" if ok else "FAIL"
                print(f"[INFLUX] {tag} {err or ''}".rstrip())
                if ok and cfg.influx_verify and not first_influx_done and influx_client is not None:
                    first_influx_done = True
                    flux_smoke_query(influx_client, cfg)

            now = time.time()
            if now - last_status >= cfg.status_interval_sec:
                st = json.dumps(status_payload(cfg, start), separators=(",", ":"))
                sinfo = client.publish(info_t, st, qos=0, retain=False)
                sinfo.wait_for_publish(timeout=5)
                print(f"[MQTT TX] {info_t} len={len(st)} rc={sinfo.rc}")
                last_status = now

            time.sleep(cfg.interval_sec)
    except KeyboardInterrupt:
        print("\nStopped.")
    finally:
        client.loop_stop()
        client.disconnect()
        if write_api is not None:
            try:
                write_api.close()
            except Exception:
                pass
        if influx_client is not None:
            influx_client.close()


if __name__ == "__main__":
    main()

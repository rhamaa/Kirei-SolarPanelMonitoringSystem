import type { InfluxChartField } from "@/lib/influx/hourly-fields";
import type { DeviceSnapshot } from "@/lib/kpi/types";

/** InfluxDB v2 tag that identifies the device (Data Explorer: `device`). */
export const INFLUX_DEVICE_TAG = "device";

/** Measurement names written by the current firmware / line protocol. */
export const INFLUX_MEASUREMENT_MPPT = "mppt";
export const INFLUX_MEASUREMENT_INVERTER = "inverter";

/** Map dashboard hourly field id → measurement + Influx `_field` name. */
export const CHART_FIELD_SOURCE: Record<
  InfluxChartField,
  { measurement: typeof INFLUX_MEASUREMENT_MPPT | typeof INFLUX_MEASUREMENT_INVERTER; influxField: string }
> = {
  mppt_charging_power: { measurement: INFLUX_MEASUREMENT_MPPT, influxField: "charging_power" },
  mppt_pv_voltage: { measurement: INFLUX_MEASUREMENT_MPPT, influxField: "pv_voltage" },
  mppt_battery_voltage: { measurement: INFLUX_MEASUREMENT_MPPT, influxField: "battery_voltage" },
  inverter_ac_power: { measurement: INFLUX_MEASUREMENT_INVERTER, influxField: "power" },
  inverter_ac_frequency: { measurement: INFLUX_MEASUREMENT_INVERTER, influxField: "frequency" },
};

/** Last row per `_field` from `group(columns:["_field"]) |> last()` → canonical pivot keys for `normalizeLatestKpi`. */
const MPPT_LAST_FIELD_TO_CANONICAL: Record<string, keyof DeviceSnapshot | null> = {
  pv_voltage: "mppt_pv_voltage",
  charging_power: "mppt_charging_power",
  charging_current: "mppt_charging_current",
  battery_voltage: "mppt_battery_voltage",
  load_current: "mppt_load_current",
  load_power: "mppt_load_power",
  fault_code: "mppt_fault_code",
};

const INVERTER_LAST_FIELD_TO_CANONICAL: Record<string, keyof DeviceSnapshot | null> = {
  voltage: "inverter_ac_voltage",
  current: "inverter_ac_current",
  power: "inverter_ac_power",
  energy: "inverter_ac_energy",
  frequency: "inverter_ac_frequency",
  pf: "inverter_ac_power_factor",
};

function rowTimeIso(row: Record<string, unknown>): string | null {
  const t = row["_time"];
  if (t instanceof Date) return t.toISOString();
  if (typeof t === "string") {
    const d = new Date(t);
    if (!Number.isNaN(d.getTime())) return d.toISOString();
  }
  return null;
}

function toFiniteNumber(raw: unknown): number | null {
  if (typeof raw === "number" && Number.isFinite(raw)) return raw;
  if (typeof raw === "string" && raw.trim() !== "") {
    const n = Number(raw);
    if (Number.isFinite(n)) return n;
  }
  return null;
}

/**
 * Merge streams from `mppt` and `inverter` last-per-field queries into one wide row
 * using `DeviceSnapshot` keys (same as legacy `pv_monitoring` pivot).
 */
export function mergeLatestFieldRowsToCanonicalRow(
  mpptRows: Array<Record<string, unknown>>,
  inverterRows: Array<Record<string, unknown>>,
): Record<string, unknown> {
  const out: Record<string, unknown> = {};
  let latestTime: string | null = null;

  const bumpTime = (iso: string | null) => {
    if (iso && (!latestTime || iso > latestTime)) latestTime = iso;
  };

  for (const r of mpptRows) {
    const field = r["_field"];
    if (typeof field !== "string") continue;
    const canon = MPPT_LAST_FIELD_TO_CANONICAL[field];
    if (!canon) continue;
    const v = toFiniteNumber(r["_value"]);
    out[canon] = v;
    bumpTime(rowTimeIso(r));
  }

  for (const r of inverterRows) {
    const field = r["_field"];
    if (typeof field !== "string") continue;
    const canon = INVERTER_LAST_FIELD_TO_CANONICAL[field];
    if (!canon) continue;
    const v = toFiniteNumber(r["_value"]);
    out[canon] = v;
    bumpTime(rowTimeIso(r));
  }

  const vAc = out["inverter_ac_voltage"];
  const iAc = out["inverter_ac_current"];
  if (typeof vAc === "number" && typeof iAc === "number" && Number.isFinite(vAc) && Number.isFinite(iAc)) {
    out["inverter_ac_apparent_power"] = vAc * iAc;
  }

  if (
    out["inverter_ac_power"] != null ||
    out["inverter_ac_voltage"] != null ||
    out["inverter_ac_frequency"] != null
  ) {
    out["inverter_valid"] = 1;
  }

  if (latestTime) out["_time"] = latestTime;
  return out;
}

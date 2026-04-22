import type { DeviceSnapshot, LatestKpiResponse, LatestKpiValue } from "@/lib/kpi/types";

import { KpiRangePreset } from "./queries";

export type { LatestKpiValue, LatestKpiResponse } from "@/lib/kpi/types";

/** Display keys returned in API `values` (stable contract for the dashboard). */
export const KPI_UNITS: Record<string, string> = {
  pv_voltage: "V",
  pv_current: "A",
  pv_power: "W",
  battery_voltage: "V",
  inverter_power: "W",
};

const KPI_KEYS = Object.keys(KPI_UNITS);

/** Influx field names from firmware line protocol (see SendDataTask). */
const KPI_FIELD_ALIASES: Record<string, string> = {
  pv_voltage: "mppt_pv_voltage",
  pv_current: "mppt_charging_current",
  pv_power: "mppt_charging_power",
  battery_voltage: "mppt_battery_voltage",
  inverter_power: "inverter_ac_power",
};

function toFiniteNumber(raw: unknown): number | null {
  if (typeof raw === "number" && Number.isFinite(raw)) return raw;
  if (typeof raw === "string" && raw.trim() !== "") {
    const n = Number(raw);
    if (Number.isFinite(n)) return n;
  }
  return null;
}

function pickField(row: Record<string, unknown> | undefined, key: string): number | null {
  if (!row) return null;
  const raw = row[key];
  if (typeof raw === "boolean") return raw ? 1 : 0;
  return toFiniteNumber(raw);
}

export function emptyDeviceSnapshot(): DeviceSnapshot {
  return {
    mppt_pv_voltage: null,
    mppt_charging_power: null,
    mppt_charging_current: null,
    mppt_battery_voltage: null,
    mppt_load_current: null,
    mppt_load_power: null,
    mppt_fault_code: null,
    inverter_valid: null,
    inverter_ac_voltage: null,
    inverter_ac_current: null,
    inverter_ac_power: null,
    inverter_ac_energy: null,
    inverter_ac_frequency: null,
    inverter_ac_power_factor: null,
    inverter_ac_apparent_power: null,
    wifi_rssi: null,
  };
}

export function extractDeviceSnapshot(row: Record<string, unknown> | undefined): DeviceSnapshot {
  return {
    mppt_pv_voltage: pickField(row, "mppt_pv_voltage"),
    mppt_charging_power: pickField(row, "mppt_charging_power"),
    mppt_charging_current: pickField(row, "mppt_charging_current"),
    mppt_battery_voltage: pickField(row, "mppt_battery_voltage"),
    mppt_load_current: pickField(row, "mppt_load_current"),
    mppt_load_power: pickField(row, "mppt_load_power"),
    mppt_fault_code: pickField(row, "mppt_fault_code"),
    inverter_valid: pickField(row, "inverter_valid"),
    inverter_ac_voltage: pickField(row, "inverter_ac_voltage"),
    inverter_ac_current: pickField(row, "inverter_ac_current"),
    inverter_ac_power: pickField(row, "inverter_ac_power"),
    inverter_ac_energy: pickField(row, "inverter_ac_energy"),
    inverter_ac_frequency: pickField(row, "inverter_ac_frequency"),
    inverter_ac_power_factor: pickField(row, "inverter_ac_power_factor"),
    inverter_ac_apparent_power: pickField(row, "inverter_ac_apparent_power"),
    wifi_rssi: pickField(row, "wifi_rssi"),
  };
}

function toIsoStringOrNull(value: unknown) {
  if (value instanceof Date) return value.toISOString();
  if (typeof value === "string") {
    const d = new Date(value);
    if (!Number.isNaN(d.getTime())) return d.toISOString();
  }
  return null;
}

export function normalizeLatestKpi(args: {
  deviceId: string;
  range: KpiRangePreset;
  rows: Array<Record<string, unknown>>;
  asOf: Date;
}): LatestKpiResponse {
  const row = args.rows[0];
  const rowTime = row ? (row["_time"] ?? row["time"]) : null;
  const time = toIsoStringOrNull(rowTime);

  const values: Record<string, LatestKpiValue> = Object.fromEntries(
    KPI_KEYS.map((key) => {
      const influxKey = KPI_FIELD_ALIASES[key] ?? key;
      const raw = row ? (row[influxKey] ?? row[key]) : undefined;
      const value = toFiniteNumber(raw);
      return [key, { value, unit: KPI_UNITS[key] ?? "", time }];
    }),
  );

  return {
    deviceId: args.deviceId,
    range: args.range,
    asOf: args.asOf.toISOString(),
    values,
    snapshot: extractDeviceSnapshot(row),
  };
}

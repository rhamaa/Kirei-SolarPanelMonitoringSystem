import { KpiRangePreset } from "./queries";

export type LatestKpiValue = {
  value: number | null;
  unit: string;
  time: string | null;
};

export type LatestKpiResponse = {
  deviceId: string;
  range: KpiRangePreset;
  asOf: string;
  values: Record<string, LatestKpiValue>;
};

const KPI_UNITS: Record<string, string> = {
  pv_voltage: "V",
  pv_current: "A",
  pv_power: "W",
  battery_voltage: "V",
  inverter_power: "W",
};

const KPI_KEYS = Object.keys(KPI_UNITS);

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
      const raw = row ? row[key] : undefined;
      const value = typeof raw === "number" && Number.isFinite(raw) ? raw : null;
      return [key, { value, unit: KPI_UNITS[key] ?? "", time }];
    }),
  );

  return {
    deviceId: args.deviceId,
    range: args.range,
    asOf: args.asOf.toISOString(),
    values,
  };
}


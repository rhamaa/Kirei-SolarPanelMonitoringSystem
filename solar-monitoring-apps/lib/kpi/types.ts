import type { KpiRangePreset } from "@/lib/influx/queries";

export type LatestKpiValue = {
  value: number | null;
  unit: string;
  time: string | null;
};

/** Latest numeric fields from Influx pivot row (firmware / `SendDataTask`). */
export type DeviceSnapshot = {
  mppt_pv_voltage: number | null;
  mppt_charging_power: number | null;
  mppt_charging_current: number | null;
  mppt_battery_voltage: number | null;
  mppt_load_current: number | null;
  mppt_load_power: number | null;
  mppt_fault_code: number | null;
  inverter_valid: number | null;
  inverter_ac_voltage: number | null;
  inverter_ac_current: number | null;
  inverter_ac_power: number | null;
  inverter_ac_energy: number | null;
  inverter_ac_frequency: number | null;
  inverter_ac_power_factor: number | null;
  inverter_ac_apparent_power: number | null;
  wifi_rssi: number | null;
};

export type LatestKpiResponse = {
  deviceId: string;
  range: KpiRangePreset;
  asOf: string;
  values: Record<string, LatestKpiValue>;
  snapshot: DeviceSnapshot;
};

/** One bucket from Influx `aggregateWindow(every: 1h, fn: mean)` (native field units). */
export type HourlyMeanPoint = {
  time: string;
  value: number | null;
};

/** Rows from Influx before attaching range / bucket metadata (added in API route). */
export type HourlySeriesCore = {
  deviceId: string;
  field: string;
  asOf: string;
  points: HourlyMeanPoint[];
};

export type HourlySeriesResponse = HourlySeriesCore & {
  range: KpiRangePreset;
  /** Flux `aggregateWindow(every: …)` duration, e.g. `2m`, `1h`. */
  aggregateEvery: string;
  /** Hours per bucket; used with mean power (W) to approximate kWh. */
  bucketDurationHours: number;
};


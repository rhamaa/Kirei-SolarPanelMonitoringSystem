import type { InfluxChartField } from "@/lib/influx/hourly-fields";
import {
  CHART_FIELD_SOURCE,
  INFLUX_DEVICE_TAG,
  INFLUX_MEASUREMENT_INVERTER,
  INFLUX_MEASUREMENT_MPPT,
} from "@/lib/influx/influx-schema";

export type KpiRangePreset = "1h" | "6h" | "24h" | "7d";

type LatestKpiFluxParams = {
  bucket: string;
  deviceId: string;
  range: KpiRangePreset;
};

function escapeFluxStringLiteral(value: string) {
  return value.replace(/\\/g, "\\\\").replace(/"/g, '\\"');
}

/** `aggregateWindow` step sized for readable buckets per dashboard range. */
export function aggregateEveryForRange(range: KpiRangePreset): string {
  switch (range) {
    case "1h":
      return "2m";
    case "6h":
      return "15m";
    case "24h":
      return "1h";
    case "7d":
      return "6h";
    default:
      return "1h";
  }
}

/** Convert Flux duration literal (`2m`, `1h`, …) to hours (for energy from mean power). */
export function parseFluxEveryToHours(every: string): number {
  const m = every.match(/^(\d+)m$/i);
  if (m) return Number(m[1]) / 60;
  const h = every.match(/^(\d+)h$/i);
  if (h) return Number(h[1]);
  const s = every.match(/^(\d+)s$/i);
  if (s) return Number(s[1]) / 3600;
  return 1;
}

function buildLatestLastPerFieldFlux(params: LatestKpiFluxParams & { measurement: string }) {
  const bucket = escapeFluxStringLiteral(params.bucket);
  const deviceId = escapeFluxStringLiteral(params.deviceId);
  const measurement = escapeFluxStringLiteral(params.measurement);

  return [
    `from(bucket: "${bucket}")`,
    `  |> range(start: -${params.range})`,
    `  |> filter(fn: (r) => r._measurement == "${measurement}")`,
    `  |> filter(fn: (r) => r["${INFLUX_DEVICE_TAG}"] == "${deviceId}")`,
    `  |> group(columns: ["_field"])`,
    `  |> last()`,
  ].join("\n");
}

/** Latest MPPT fields (measurement `mppt`, tag `device`). */
export function buildLatestMpptLastFieldsFlux(params: LatestKpiFluxParams) {
  return buildLatestLastPerFieldFlux({ ...params, measurement: INFLUX_MEASUREMENT_MPPT });
}

/** Latest inverter fields (measurement `inverter`, tag `device`). */
export function buildLatestInverterLastFieldsFlux(params: LatestKpiFluxParams) {
  return buildLatestLastPerFieldFlux({ ...params, measurement: INFLUX_MEASUREMENT_INVERTER });
}

/** Mean of a numeric field over `range`, bucketed by `aggregateEveryForRange(range)`. */
export type HourlyMeanFluxParams = {
  bucket: string;
  deviceId: string;
  field: InfluxChartField;
  range: KpiRangePreset;
};

export function buildHourlyMeanFlux(params: HourlyMeanFluxParams) {
  const bucket = escapeFluxStringLiteral(params.bucket);
  const deviceId = escapeFluxStringLiteral(params.deviceId);
  const every = aggregateEveryForRange(params.range);
  const spec = CHART_FIELD_SOURCE[params.field];
  const measurement = escapeFluxStringLiteral(spec.measurement);
  const influxField = escapeFluxStringLiteral(spec.influxField);

  return [
    `from(bucket: "${bucket}")`,
    `  |> range(start: -${params.range})`,
    `  |> filter(fn: (r) => r._measurement == "${measurement}")`,
    `  |> filter(fn: (r) => r["${INFLUX_DEVICE_TAG}"] == "${deviceId}")`,
    `  |> filter(fn: (r) => r._field == "${influxField}")`,
    `  |> aggregateWindow(every: ${every}, fn: mean, createEmpty: true)`,
  ].join("\n");
}

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

export function buildLatestKpiFlux(params: LatestKpiFluxParams) {
  const bucket = escapeFluxStringLiteral(params.bucket);
  const deviceId = escapeFluxStringLiteral(params.deviceId);

  return [
    `from(bucket: "${bucket}")`,
    `  |> range(start: -${params.range})`,
    `  |> filter(fn: (r) => r._measurement == "pv_monitoring")`,
    `  |> filter(fn: (r) => r.device_id == "${deviceId}")`,
    `  |> group(columns: ["device_id", "_field"])`,
    `  |> last()`,
    `  |> pivot(rowKey: ["_time", "device_id"], columnKey: ["_field"], valueColumn: "_value")`,
  ].join("\n");
}

/** Mean of a numeric field over `range`, bucketed by `aggregateEveryForRange(range)`. */
export type HourlyMeanFluxParams = {
  bucket: string;
  deviceId: string;
  field: string;
  range: KpiRangePreset;
};

export function buildHourlyMeanFlux(params: HourlyMeanFluxParams) {
  const bucket = escapeFluxStringLiteral(params.bucket);
  const deviceId = escapeFluxStringLiteral(params.deviceId);
  const field = escapeFluxStringLiteral(params.field);
  const every = aggregateEveryForRange(params.range);

  return [
    `from(bucket: "${bucket}")`,
    `  |> range(start: -${params.range})`,
    `  |> filter(fn: (r) => r._measurement == "pv_monitoring")`,
    `  |> filter(fn: (r) => r.device_id == "${deviceId}")`,
    `  |> filter(fn: (r) => r._field == "${field}")`,
    `  |> aggregateWindow(every: ${every}, fn: mean, createEmpty: true)`,
  ].join("\n");
}


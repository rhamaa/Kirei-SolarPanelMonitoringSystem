export type KpiRangePreset = "1h" | "6h" | "24h" | "7d";

type LatestKpiFluxParams = {
  bucket: string;
  deviceId: string;
  range: KpiRangePreset;
};

function escapeFluxStringLiteral(value: string) {
  return value.replace(/\\/g, "\\\\").replace(/"/g, '\\"');
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


import type { KpiRangePreset } from "@/lib/influx/queries";

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


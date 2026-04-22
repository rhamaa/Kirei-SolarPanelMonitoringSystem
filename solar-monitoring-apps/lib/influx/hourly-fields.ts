/** Allowed `_field` names for `/api/kpi/hourly` (must match firmware line protocol). */
export const INFLUX_CHART_FIELDS = [
  "mppt_charging_power",
  "inverter_ac_power",
  "mppt_battery_voltage",
  "inverter_ac_frequency",
  "mppt_pv_voltage",
] as const;

export type InfluxChartField = (typeof INFLUX_CHART_FIELDS)[number];

import type { KpiRangePreset } from "@/lib/influx/queries";

/** Influx uses UTC; charts show operator time in WIB. */
export const CHART_DISPLAY_TIMEZONE = "Asia/Jakarta" as const;

const clockHm: Intl.DateTimeFormatOptions = {
  timeZone: CHART_DISPLAY_TIMEZONE,
  hour: "2-digit",
  minute: "2-digit",
  hour12: false,
  hourCycle: "h23",
};

const clockDate: Intl.DateTimeFormatOptions = {
  timeZone: CHART_DISPLAY_TIMEZONE,
  day: "numeric",
  month: "short",
};

/** X-axis tick and row `label` for Influx-backed Recharts. */
export function formatInfluxTick(ms: number, range: KpiRangePreset): string {
  const d = new Date(ms);
  if (Number.isNaN(d.getTime())) return "—";
  if (range === "7d") {
    return d.toLocaleDateString("en-GB", clockDate);
  }
  return d.toLocaleTimeString("en-GB", clockHm);
}

/** Full timestamp for tooltips (ISO instant → local WIB string). */
export function formatInfluxIsoTooltipWib(iso: string): string {
  const d = new Date(iso);
  if (Number.isNaN(d.getTime())) return "";
  return (
    d.toLocaleString("en-GB", {
      timeZone: CHART_DISPLAY_TIMEZONE,
      year: "numeric",
      month: "2-digit",
      day: "2-digit",
      ...clockHm,
      second: "2-digit",
    }) + " WIB"
  );
}

/** Local clock hour 0–23 in WIB for a bucket’s start time. */
export function clockHourWibFromIso(iso: string): number | null {
  const d = new Date(iso);
  if (Number.isNaN(d.getTime())) return null;
  const parts = new Intl.DateTimeFormat("en-GB", {
    timeZone: CHART_DISPLAY_TIMEZONE,
    hour: "2-digit",
    hour12: false,
    hourCycle: "h23",
  }).formatToParts(d);
  const h = parts.find((p) => p.type === "hour")?.value;
  return h != null ? parseInt(h, 10) : null;
}

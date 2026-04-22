import type { HourlyMeanPoint, HourlySeriesCore } from "@/lib/kpi/types";

function toFiniteNumber(raw: unknown): number | null {
  if (typeof raw === "number" && Number.isFinite(raw)) return raw;
  if (typeof raw === "string" && raw.trim() !== "") {
    const n = Number(raw);
    if (Number.isFinite(n)) return n;
  }
  return null;
}

function rowTimeIso(row: Record<string, unknown>): string | null {
  const t = row["_time"] ?? row["_start"] ?? row["time"];
  if (t instanceof Date) return t.toISOString();
  if (typeof t === "string") {
    const d = new Date(t);
    if (!Number.isNaN(d.getTime())) return d.toISOString();
  }
  return null;
}

export function normalizeHourlyMeanSeries(args: {
  deviceId: string;
  field: string;
  rows: Array<Record<string, unknown>>;
  asOf: Date;
}): HourlySeriesCore {
  const points: HourlyMeanPoint[] = args.rows
    .map((r) => {
      const time = rowTimeIso(r);
      if (!time) return null;
      const value = toFiniteNumber(r["_value"]);
      return { time, value };
    })
    .filter((p): p is HourlyMeanPoint => p !== null)
    .sort((a, b) => a.time.localeCompare(b.time));

  return {
    deviceId: args.deviceId,
    field: args.field,
    asOf: args.asOf.toISOString(),
    points,
  };
}

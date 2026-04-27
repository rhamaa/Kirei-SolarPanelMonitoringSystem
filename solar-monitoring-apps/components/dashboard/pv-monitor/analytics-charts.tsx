"use client";

import type { CSSProperties } from "react";
import { useId, useMemo } from "react";
import {
  Area,
  AreaChart,
  Bar,
  BarChart,
  CartesianGrid,
  Legend,
  ResponsiveContainer,
  Tooltip,
  XAxis,
  YAxis,
} from "recharts";

import type { KpiRangePreset } from "@/lib/influx/queries";
import { formatInfluxIsoTooltipWib, formatInfluxTick } from "@/lib/kpi/chart-timezone";
import type { HourlySeriesResponse } from "@/lib/kpi/types";

import { fmt } from "./chart-kit";

type AxisVariant = "power" | "voltage" | "frequency";

export type SeriesChartRow = {
  /** X: epoch ms (Influx) or sample index 0..n (poll fallback). */
  x: number;
  /** Short label for axis / tooltip. */
  label: string;
  y: number;
};

/** Prefer Influx buckets; otherwise poll ring buffer (index-based X). */
export function seriesChartRows(
  series: HourlySeriesResponse | undefined,
  fallback: number[],
  range: KpiRangePreset,
): { rows: SeriesChartRow[]; source: "influx" | "poll" } {
  const pts = series?.points;
  if (pts && pts.length > 1) {
    return {
      source: "influx",
      rows: pts.map((p) => {
        const ms = Date.parse(p.time);
        const x = Number.isFinite(ms) ? ms : 0;
        const y = p.value != null && Number.isFinite(p.value) ? p.value : 0;
        return { x, label: formatInfluxTick(x, range), y };
      }),
    };
  }
  if (fallback.length > 1) {
    const n = fallback.length;
    return {
      source: "poll",
      rows: fallback.map((v, i) => ({
        x: i,
        label: `#${i + 1}/${n}`,
        y: Number.isFinite(v) ? v : 0,
      })),
    };
  }
  return { source: "poll", rows: [] };
}

function yDomainForVariant(variant: AxisVariant, rows: SeriesChartRow[]): [number, number] | undefined {
  if (rows.length < 2) return undefined;
  const vals = rows.map((r) => r.y);
  const lo = Math.min(...vals);
  const hi = Math.max(...vals);
  if (variant === "power") return [0, Math.max(hi * 1.12, 1e-6)];
  if (variant === "frequency") {
    const pad = Math.max(0.15, (hi - lo) * 0.08);
    return [lo - pad, hi + pad];
  }
  const span = hi - lo || 1;
  return [lo - span * 0.04, hi + span * 0.04];
}

const axisStyle = { fontSize: 11, fontFamily: "var(--font-app-mono)", fill: "var(--muted)" };
const gridStroke = "rgba(255,255,255,0.06)";

function tooltipBoxStyle(): CSSProperties {
  return {
    background: "var(--bg3)",
    border: "1px solid var(--border)",
    borderRadius: 10,
    fontFamily: "var(--font-app-mono)",
    fontSize: 11,
    color: "var(--text)",
  };
}

export function AnalyticsAreaPanel(props: {
  title: string;
  unit: string;
  color: string;
  variant: AxisVariant;
  series: HourlySeriesResponse | undefined;
  fallback: number[];
  range: KpiRangePreset;
  stat: string;
  statHint: string;
  foot: string;
}) {
  const { title, unit, color, variant, series, fallback, range, stat, statHint, foot } = props;
  const gid = useId().replace(/:/g, "");
  const { rows, source } = useMemo(() => seriesChartRows(series, fallback, range), [series, fallback, range]);
  const domain = useMemo(() => yDomainForVariant(variant, rows), [variant, rows]);
  const tickFmt = (v: number) => (source === "influx" ? formatInfluxTick(v, range) : `#${Math.round(v) + 1}`);

  if (rows.length < 2) {
    return (
      <div className="rounded-xl border p-5 md:p-6" style={{ background: "var(--bg2)", borderColor: "var(--border)" }}>
        <div className="mb-3.5 flex items-start justify-between gap-3">
          <div>
            <div className="mb-1 text-[11px] font-bold uppercase tracking-[0.1em]" style={{ fontFamily: "var(--font-app-mono)", color: "var(--muted)" }}>
              {title}
            </div>
            <div className="text-[22px] font-bold" style={{ fontFamily: "var(--font-app-mono)", color }}>
              {stat}
            </div>
            <div className="text-[10px]" style={{ color: "var(--muted)" }}>
              {statHint}
            </div>
          </div>
          <span
            className="shrink-0 rounded-md px-2 py-0.5 text-[10px] font-bold"
            style={{
              fontFamily: "var(--font-app-mono)",
              color,
              background: `${color}15`,
              border: `1px solid ${color}30`,
            }}
          >
            {unit}
          </span>
        </div>
        <div className="flex h-[200px] items-center justify-center rounded-lg border text-xs" style={{ borderColor: "var(--border)", color: "var(--muted)", fontFamily: "var(--font-app-mono)" }}>
          Collecting samples… refresh a few times
        </div>
        <div className="mt-2 text-[9px]" style={{ color: "var(--muted)", fontFamily: "var(--font-app-mono)" }}>
          {foot}
        </div>
      </div>
    );
  }

  return (
    <div className="rounded-xl border p-5 md:p-6" style={{ background: "var(--bg2)", borderColor: "var(--border)" }}>
      <div className="mb-3.5 flex items-start justify-between gap-3">
        <div>
          <div className="mb-1 text-[11px] font-bold uppercase tracking-[0.1em]" style={{ fontFamily: "var(--font-app-mono)", color: "var(--muted)" }}>
            {title}
          </div>
          <div className="text-[22px] font-bold" style={{ fontFamily: "var(--font-app-mono)", color }}>
            {stat}
          </div>
          <div className="text-[10px]" style={{ color: "var(--muted)" }}>
            {statHint}
          </div>
        </div>
        <span
          className="shrink-0 rounded-md px-2 py-0.5 text-[10px] font-bold"
          style={{
            fontFamily: "var(--font-app-mono)",
            color,
            background: `${color}15`,
            border: `1px solid ${color}30`,
          }}
        >
          {unit}
        </span>
      </div>

      <div className="h-[200px] w-full min-w-0">
        <ResponsiveContainer width="100%" height="100%">
          <AreaChart data={rows} margin={{ top: 4, right: 8, left: 0, bottom: 0 }}>
            <defs>
              <linearGradient id={`a-${gid}`} x1="0" y1="0" x2="0" y2="1">
                <stop offset="0%" stopColor={color} stopOpacity={0.35} />
                <stop offset="100%" stopColor={color} stopOpacity={0.02} />
              </linearGradient>
            </defs>
            <CartesianGrid strokeDasharray="3 3" stroke={gridStroke} vertical={false} />
            <XAxis
              dataKey="x"
              type="number"
              domain={["dataMin", "dataMax"]}
              tickFormatter={tickFmt}
              tick={axisStyle}
              axisLine={{ stroke: "var(--border)" }}
              tickLine={{ stroke: "var(--border)" }}
              minTickGap={28}
            />
            <YAxis
              domain={domain ?? ["auto", "auto"]}
              tick={axisStyle}
              axisLine={false}
              tickLine={false}
              width={44}
              tickFormatter={(v) => (variant === "frequency" ? fmt(v, 2) : variant === "voltage" ? fmt(v, 2) : fmt(v, 0))}
            />
            <Tooltip
              contentStyle={tooltipBoxStyle()}
              labelStyle={{ color: "var(--muted)", fontFamily: "var(--font-app-mono)", fontSize: 10 }}
              formatter={(value, name) => {
                const v = typeof value === "number" ? value : value != null ? Number(value) : NaN;
                const digits = variant === "frequency" ? 3 : variant === "voltage" ? 2 : 1;
                const text = Number.isFinite(v) ? `${fmt(v, digits)} ${unit}` : "—";
                return [text, typeof name === "string" ? name : title];
              }}
              labelFormatter={(_, p) => {
                const row = (Array.isArray(p) ? p[0]?.payload : undefined) as SeriesChartRow | undefined;
                return row?.label ?? "";
              }}
            />
            <Area type="monotone" dataKey="y" name={title} stroke={color} strokeWidth={2} fill={`url(#a-${gid})`} isAnimationActive={false} />
          </AreaChart>
        </ResponsiveContainer>
      </div>

      <div className="mt-2 flex justify-between gap-2 text-[9px]" style={{ color: "var(--muted)", fontFamily: "var(--font-app-mono)" }}>
        <span className="min-w-0 truncate">{foot}</span>
        <span className="shrink-0">{source === "influx" ? "WIB" : "poll idx"}</span>
      </div>
    </div>
  );
}

type DualRow = { x: number; label: string; charging: number; ac: number };

function dualPowerRows(
  sChg: HourlySeriesResponse | undefined,
  sAc: HourlySeriesResponse | undefined,
  fbChg: number[],
  fbAc: number[],
  range: KpiRangePreset,
): { rows: DualRow[]; source: "influx" | "poll" } {
  const pc = sChg?.points;
  const pa = sAc?.points;
  if (pc && pa && pc.length > 1 && pa.length > 1) {
    const mapAc = new Map(pa.map((p) => [p.time, p.value]));
    return {
      source: "influx",
      rows: pc.map((p) => {
        const ms = Date.parse(p.time);
        const x = Number.isFinite(ms) ? ms : 0;
        const av = mapAc.get(p.time);
        return {
          x,
          label: formatInfluxTick(x, range),
          charging: p.value != null && Number.isFinite(p.value) ? p.value : 0,
          ac: av != null && Number.isFinite(av) ? av : 0,
        };
      }),
    };
  }
  const n = Math.min(fbChg.length, fbAc.length);
  if (n > 1) {
    const chg = fbChg.slice(-n);
    const ac = fbAc.slice(-n);
    return {
      source: "poll",
      rows: chg.map((v, i) => ({
        x: i,
        label: `#${i + 1}/${n}`,
        charging: Number.isFinite(v) ? v : 0,
        ac: Number.isFinite(ac[i]) ? ac[i] : 0,
      })),
    };
  }
  return { source: "poll", rows: [] };
}

export function AnalyticsPowerCompare(props: {
  range: KpiRangePreset;
  charging: HourlySeriesResponse | undefined;
  ac: HourlySeriesResponse | undefined;
  histChg: number[];
  histAc: number[];
}) {
  const { range, charging, ac, histChg, histAc } = props;
  const gid = useId().replace(/:/g, "");
  const { rows, source } = useMemo(() => dualPowerRows(charging, ac, histChg, histAc, range), [charging, ac, histChg, histAc, range]);
  const dualYMax = useMemo(() => {
    let m = 1;
    for (const r of rows) m = Math.max(m, r.charging, r.ac);
    return m * 1.12;
  }, [rows]);

  if (rows.length < 2) {
    return (
      <div className="rounded-xl border p-5 md:p-6" style={{ background: "var(--bg2)", borderColor: "var(--border)" }}>
        <div className="mb-2 text-[11px] font-bold uppercase tracking-[0.1em]" style={{ fontFamily: "var(--font-app-mono)", color: "var(--muted)" }}>
          Charging vs AC output
        </div>
        <div className="flex h-[220px] items-center justify-center rounded-lg border text-xs" style={{ borderColor: "var(--border)", color: "var(--muted)", fontFamily: "var(--font-app-mono)" }}>
          Need two power series to compare — widen range or wait for Influx buckets.
        </div>
      </div>
    );
  }

  const foot =
    source === "influx"
      ? `Influx · −${charging?.range ?? range} · bucket ${charging?.aggregateEvery ?? ac?.aggregateEvery ?? "—"}`
      : `Poll buffer · last ${rows.length} samples`;

  return (
    <div className="rounded-xl border p-5 md:p-6" style={{ background: "var(--bg2)", borderColor: "var(--border)" }}>
      <div className="mb-1 flex flex-col gap-1 sm:flex-row sm:items-end sm:justify-between">
        <div>
          <div className="text-[11px] font-bold uppercase tracking-[0.1em]" style={{ fontFamily: "var(--font-app-mono)", color: "var(--muted)" }}>
            Charging vs AC output
          </div>
          <div className="text-[10px]" style={{ color: "var(--muted)", fontFamily: "var(--font-app-mono)" }}>
            Mean power per bucket (W) — same time window as Range
          </div>
        </div>
      </div>

      <div className="mt-2 h-[260px] w-full min-w-0">
        <ResponsiveContainer width="100%" height="100%">
          <AreaChart data={rows} margin={{ top: 8, right: 12, left: 0, bottom: 4 }}>
            <defs>
              <linearGradient id={`pc-${gid}`} x1="0" y1="0" x2="0" y2="1">
                <stop offset="0%" stopColor="var(--accent)" stopOpacity={0.3} />
                <stop offset="100%" stopColor="var(--accent)" stopOpacity={0.02} />
              </linearGradient>
              <linearGradient id={`pa-${gid}`} x1="0" y1="0" x2="0" y2="1">
                <stop offset="0%" stopColor="var(--cyan)" stopOpacity={0.28} />
                <stop offset="100%" stopColor="var(--cyan)" stopOpacity={0.02} />
              </linearGradient>
            </defs>
            <CartesianGrid strokeDasharray="3 3" stroke={gridStroke} vertical={false} />
            <XAxis
              dataKey="x"
              type="number"
              domain={["dataMin", "dataMax"]}
              tickFormatter={(v) => (source === "influx" ? formatInfluxTick(v, range) : `#${Math.round(v) + 1}`)}
              tick={axisStyle}
              axisLine={{ stroke: "var(--border)" }}
              tickLine={{ stroke: "var(--border)" }}
              minTickGap={32}
            />
            <YAxis domain={[0, dualYMax]} tick={axisStyle} axisLine={false} tickLine={false} width={48} tickFormatter={(v) => fmt(v, 0)} />
            <Tooltip
              contentStyle={tooltipBoxStyle()}
              labelStyle={{ color: "var(--muted)", fontFamily: "var(--font-app-mono)", fontSize: 10 }}
              labelFormatter={(_, p) => {
                const row = (Array.isArray(p) ? p[0]?.payload : undefined) as DualRow | undefined;
                return row?.label ?? "";
              }}
              formatter={(value, name) => {
                const v = typeof value === "number" ? value : value != null ? Number(value) : NaN;
                const text = Number.isFinite(v) ? `${fmt(v, 1)} W` : "—";
                return [text, typeof name === "string" ? name : ""];
              }}
            />
            <Legend wrapperStyle={{ fontFamily: "var(--font-app-mono)", fontSize: 11, paddingTop: 8, color: "var(--muted)" }} />
            <Area
              type="monotone"
              dataKey="charging"
              name="PV charging"
              stroke="var(--accent)"
              strokeWidth={2}
              fill={`url(#pc-${gid})`}
              isAnimationActive={false}
            />
            <Area type="monotone" dataKey="ac" name="AC output" stroke="var(--cyan)" strokeWidth={2} fill={`url(#pa-${gid})`} isAnimationActive={false} />
          </AreaChart>
        </ResponsiveContainer>
      </div>
      <div className="mt-1 text-[9px]" style={{ color: "var(--muted)", fontFamily: "var(--font-app-mono)" }}>
        {foot}
      </div>
    </div>
  );
}

type BarEnergyRow = { x: number; label: string; kwh: number; meanW: number; iso: string };

export function AnalyticsChargingEnergyBar(props: {
  series: HourlySeriesResponse | undefined;
  range: KpiRangePreset;
  kwhTotal: number | null;
  peakWibHour: number | null;
  peakMeanW: string;
}) {
  const { series, range, kwhTotal, peakWibHour, peakMeanW } = props;
  const bucketH = series?.bucketDurationHours ?? 0;

  const rows: BarEnergyRow[] = useMemo(() => {
    const pts = series?.points ?? [];
    if (!pts.length) return [];
    return pts.map((p) => {
      const ms = Date.parse(p.time);
      const x = Number.isFinite(ms) ? ms : 0;
      const meanW = p.value != null && Number.isFinite(p.value) ? p.value : 0;
      const kwh = (meanW * bucketH) / 1000;
      return { x, label: formatInfluxTick(x, range), kwh, meanW, iso: p.time };
    });
  }, [series, bucketH, range]);

  const maxKwh = useMemo(() => Math.max(...rows.map((r) => r.kwh), 1e-9), [rows]);

  if (!rows.length) {
    return (
      <div className="rounded-xl border px-5 py-5 md:px-6" style={{ background: "var(--bg2)", borderColor: "var(--border)" }}>
        <div className="mb-3 text-[11px] font-bold uppercase tracking-[0.1em]" style={{ fontFamily: "var(--font-app-mono)", color: "var(--muted)" }}>
          Charging energy by bucket (Influx)
        </div>
        <div className="flex h-[200px] items-center justify-center rounded-lg border text-xs" style={{ borderColor: "var(--border)", color: "var(--muted)", fontFamily: "var(--font-app-mono)" }}>
          No hourly series yet (Influx empty or hourly query failed).
        </div>
      </div>
    );
  }

  return (
    <div className="rounded-xl border px-5 py-5 md:px-6" style={{ background: "var(--bg2)", borderColor: "var(--border)" }}>
      <div className="mb-4 flex flex-col justify-between gap-3 sm:flex-row sm:items-center">
        <div>
          <div className="mb-1 text-[11px] font-bold uppercase tracking-[0.1em]" style={{ fontFamily: "var(--font-app-mono)", color: "var(--muted)" }}>
            Charging energy by bucket (Influx)
          </div>
          <div className="text-[22px] font-bold" style={{ fontFamily: "var(--font-app-mono)", color: "var(--accent)" }}>
            {kwhTotal != null ? `${fmt(kwhTotal, 2)} kWh` : "—"}
          </div>
          <div className="text-[11px]" style={{ color: "var(--muted)" }}>
            Σ kWh ≈ Σ(mean W × {fmt(bucketH, 4)} h) ÷ 1000 · −{series?.range ?? range} · every{" "}
            <code className="rounded bg-white/5 px-1" style={{ fontFamily: "var(--font-app-mono)" }}>
              {series?.aggregateEvery ?? "—"}
            </code>
          </div>
        </div>
        <div className="text-left sm:text-right">
          <div className="mb-1 text-[10px]" style={{ color: "var(--muted)", fontFamily: "var(--font-app-mono)" }}>
            Peak bucket (WIB)
          </div>
          <div className="text-base font-bold" style={{ fontFamily: "var(--font-app-mono)", color: "var(--accent)" }}>
            {peakWibHour != null ? `${String(peakWibHour).padStart(2, "0")}:00` : "—"}
          </div>
          <div className="text-[10px]" style={{ color: "var(--muted)" }}>
            Peak mean: {peakMeanW}
          </div>
        </div>
      </div>

      <div className="h-[240px] w-full min-w-0">
        <ResponsiveContainer width="100%" height="100%">
          <BarChart data={rows} margin={{ top: 4, right: 8, left: 0, bottom: 4 }} barCategoryGap="12%">
            <CartesianGrid strokeDasharray="3 3" stroke={gridStroke} vertical={false} />
            <XAxis
              dataKey="x"
              type="number"
              domain={["dataMin", "dataMax"]}
              tickFormatter={(v) => formatInfluxTick(v, range)}
              tick={axisStyle}
              axisLine={{ stroke: "var(--border)" }}
              tickLine={{ stroke: "var(--border)" }}
              minTickGap={20}
            />
            <YAxis
              domain={[0, maxKwh * 1.15]}
              tick={axisStyle}
              axisLine={false}
              tickLine={false}
              width={44}
              tickFormatter={(v) => fmt(v, 2)}
              label={{ value: "kWh / bucket", angle: -90, position: "insideLeft", fill: "var(--muted)", fontSize: 10, fontFamily: "var(--font-app-mono)" }}
            />
            <Tooltip
              contentStyle={tooltipBoxStyle()}
              labelFormatter={(_, p) => {
                const pl = Array.isArray(p) ? p[0] : undefined;
                const row = pl?.payload as BarEnergyRow | undefined;
                return row?.iso ? formatInfluxIsoTooltipWib(row.iso) : "";
              }}
              formatter={(value, _name, item) => {
                const row = item?.payload as BarEnergyRow | undefined;
                const k = typeof value === "number" ? value : value != null ? Number(value) : NaN;
                const w = row?.meanW;
                const line1 = Number.isFinite(k) ? `${fmt(k, 3)} kWh / bucket` : "—";
                const line2 = w != null && Number.isFinite(w) ? `${fmt(w, 1)} W mean` : "";
                return [line1, line2].filter(Boolean).join(" · ");
              }}
            />
            <Bar dataKey="kwh" name="Energy" fill="var(--accent)" fillOpacity={0.72} radius={[4, 4, 0, 0]} isAnimationActive={false} />
          </BarChart>
        </ResponsiveContainer>
      </div>
    </div>
  );
}

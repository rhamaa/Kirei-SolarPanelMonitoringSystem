"use client";

import { useEffect, useMemo, useState } from "react";

import type { InfluxChartField } from "@/lib/influx/hourly-fields";
import type { KpiRangePreset } from "@/lib/influx/queries";
import { clockHourWibFromIso } from "@/lib/kpi/chart-timezone";
import type { DeviceSnapshot, HourlySeriesResponse, LatestKpiResponse } from "@/lib/kpi/types";
import { emptyDeviceSnapshot } from "@/lib/influx/normalize";

import { AnalyticsAreaPanel, AnalyticsChargingEnergyBar, AnalyticsPowerCompare } from "./analytics-charts";
import {
  BatterySocBar,
  EnergyFlowPanel,
  Sparkline,
  StatCard,
  StatusBadge,
  clamp,
  fmt,
} from "./chart-kit";

export type PvHistory = {
  pv_voltage: number[];
  pv_power: number[];
  battery: number[];
  ac_power: number[];
  ac_freq: number[];
};

function snap(data: LatestKpiResponse | null): DeviceSnapshot {
  return data?.snapshot ?? emptyDeviceSnapshot();
}

function formatAsOfUtc(iso: string | undefined): string {
  if (!iso) return "—";
  const d = new Date(iso);
  if (Number.isNaN(d.getTime())) return "—";
  return `${d.toISOString().replace("T", " ").slice(0, 19)} UTC`;
}

/** kWh from mean power (W) per bucket: Σ(mean_W × bucketHours) / 1000. */
function kWhFromPowerMeans(series: HourlySeriesResponse | undefined): number | null {
  if (!series?.points.length) return null;
  if (series.field !== "mppt_charging_power" && series.field !== "inverter_ac_power") return null;
  const h = series.bucketDurationHours;
  const sumWh = series.points.reduce((s, p) => s + (p.value != null && Number.isFinite(p.value) ? p.value * h : 0), 0);
  return sumWh / 1000;
}

function seriesValuesForChart(s: HourlySeriesResponse | undefined): number[] {
  return (s?.points ?? []).map((p) => (p.value != null && Number.isFinite(p.value) ? p.value : 0));
}

function historyOrSeries(hourlySeries: HourlySeriesResponse | undefined, fallback: number[]): number[] | undefined {
  const v = seriesValuesForChart(hourlySeries);
  if (v.length > 1) return v;
  return fallback.length > 1 ? fallback : undefined;
}

export function PvMonitorChrome(props: {
  deviceId: string;
  setDeviceId: (v: string) => void;
  deviceIdError: string | null;
  range: KpiRangePreset;
  setRange: (v: KpiRangePreset) => void;
  rangePresets: Array<{ value: KpiRangePreset; label: string }>;
  onRefresh: () => void;
  canRefresh: boolean;
  loading: boolean;
  error: string | null;
  data: LatestKpiResponse | null;
  /** Per-field mean buckets from Influx (`/api/kpi/hourly?range=…`). */
  hourlyByField: Partial<Record<InfluxChartField, HourlySeriesResponse>>;
  isEmpty: boolean;
  hist: PvHistory;
  chartWindow: number;
  updateIntervalMs: number;
}) {
  const {
    deviceId,
    setDeviceId,
    deviceIdError,
    range,
    setRange,
    rangePresets,
    onRefresh,
    canRefresh,
    loading,
    error,
    data,
    hourlyByField,
    isEmpty,
    hist,
    chartWindow,
    updateIntervalMs,
  } = props;

  const [tab, setTab] = useState<"dashboard" | "analytics" | "inverter" | "system">("dashboard");
  /** Avoid `new Date()` during SSR — server and client clocks differ and cause hydration errors. */
  const [now, setNow] = useState<Date | null>(null);

  useEffect(() => {
    const tick = () => setNow(new Date());
    tick();
    const id = window.setInterval(tick, 1000);
    return () => window.clearInterval(id);
  }, []);

  const s = snap(data);
  const pvV = s.mppt_pv_voltage;
  const chgW = s.mppt_charging_power;
  const chgA = s.mppt_charging_current;
  const batV = s.mppt_battery_voltage;
  const batSoc = s.mppt_battery_soc;
  const acW = s.inverter_ac_power;
  const acV = s.inverter_ac_voltage;
  const acI = s.inverter_ac_current;
  const acHz = s.inverter_ac_frequency;
  const acE = s.inverter_ac_energy;
  const pf = s.inverter_ac_power_factor;
  const apparent = s.inverter_ac_apparent_power;
  const loadW = s.mppt_load_power;
  const fault = s.mppt_fault_code;

  const chargingHourly = hourlyByField["mppt_charging_power"];
  const hourlyValues = useMemo(() => seriesValuesForChart(chargingHourly), [chargingHourly]);
  const kwhFromInfluxHourly = useMemo(() => kWhFromPowerMeans(chargingHourly), [chargingHourly]);
  const peakIdx = useMemo(() => {
    if (!hourlyValues.length) return -1;
    let max = -Infinity;
    let idx = 0;
    hourlyValues.forEach((v, i) => {
      if (v > max) {
        max = v;
        idx = i;
      }
    });
    return idx;
  }, [hourlyValues]);
  const peakWibHour = useMemo(() => {
    const pts = chargingHourly?.points ?? [];
    if (peakIdx < 0 || !pts[peakIdx]) return null;
    return clockHourWibFromIso(pts[peakIdx].time);
  }, [chargingHourly, peakIdx]);

  const mpptCharging = (chgW ?? 0) > 1;

  const tabs = ["dashboard", "analytics", "inverter", "system"] as const;

  return (
    <div className="pb-10" style={{ minHeight: "100vh" }}>
      <header
        className="sticky top-0 z-20 flex h-[58px] items-center justify-between border-b px-4 backdrop-blur md:px-7"
        style={{ borderColor: "var(--border)", background: "rgba(8,12,16,0.95)" }}
      >
        <div className="flex items-center gap-4">
          <div
            className="flex h-8 w-8 items-center justify-center rounded-lg border text-base"
            style={{
              borderColor: "color-mix(in srgb, var(--accent) 35%, transparent)",
              background:
                "linear-gradient(135deg, color-mix(in srgb, var(--accent) 25%, transparent), color-mix(in srgb, var(--accent) 8%, transparent))",
            }}
          >
            ☀️
          </div>
          <div>
            <div className="text-[13px] font-bold tracking-[0.08em]" style={{ fontFamily: "var(--font-app-mono)", color: "var(--accent)" }}>
              PV MONITOR
            </div>
            <div className="text-[10px]" style={{ fontFamily: "var(--font-app-mono)", color: "var(--muted)" }}>
              {deviceId.trim() || "—"}
            </div>
          </div>
        </div>

        <nav className="hidden flex-1 justify-center gap-1 sm:flex">
          {tabs.map((t) => (
            <button
              key={t}
              type="button"
              onClick={() => setTab(t)}
              className="rounded-lg px-3.5 py-1.5 text-[11px] font-bold uppercase tracking-[0.08em] transition-colors"
              style={{
                fontFamily: "var(--font-app-mono)",
                background: tab === t ? "color-mix(in srgb, var(--accent) 12%, transparent)" : "transparent",
                border: tab === t ? "1px solid color-mix(in srgb, var(--accent) 35%, transparent)" : "1px solid transparent",
                color: tab === t ? "var(--accent)" : "var(--muted)",
              }}
            >
              {t}
            </button>
          ))}
        </nav>

        <div className="flex items-center gap-3 md:gap-4">
          <StatusBadge text={mpptCharging ? "MPPT Charging" : "Idle"} ok={mpptCharging} />
          <div className="text-[10px] tabular-nums" style={{ fontFamily: "var(--font-app-mono)", color: "var(--muted)" }}>
            {now
              ? now.toLocaleTimeString("id-ID", { hour: "2-digit", minute: "2-digit", second: "2-digit" }).replace(/\./g, ":")
              : "—:—:—"}
          </div>
        </div>
      </header>

      <div className="flex gap-1 overflow-x-auto border-b px-4 py-2 sm:hidden" style={{ borderColor: "var(--border)", background: "rgba(8,12,16,0.85)" }}>
        {tabs.map((t) => (
          <button
            key={t}
            type="button"
            onClick={() => setTab(t)}
            className="shrink-0 rounded-lg px-3 py-1.5 text-[11px] font-bold uppercase tracking-[0.08em]"
            style={{
              fontFamily: "var(--font-app-mono)",
              background: tab === t ? "color-mix(in srgb, var(--accent) 12%, transparent)" : "transparent",
              border: tab === t ? "1px solid color-mix(in srgb, var(--accent) 35%, transparent)" : "1px solid transparent",
              color: tab === t ? "var(--accent)" : "var(--muted)",
            }}
          >
            {t}
          </button>
        ))}
      </div>

      <div
        className="mx-auto grid max-w-[1200px] gap-3 border-b px-4 py-3 md:grid-cols-[1fr_120px_140px] md:px-7"
        style={{ borderColor: "var(--border)", background: "rgba(8,12,16,0.55)" }}
      >
        <label className="block min-w-0">
          <div className="text-[11px] font-semibold uppercase tracking-wide" style={{ color: "var(--muted)", fontFamily: "var(--font-app-mono)" }}>
            Device ID
          </div>
          <input
            type="text"
            value={deviceId}
            onChange={(e) => setDeviceId(e.target.value)}
            className="mt-1 h-10 w-full min-w-0 rounded-lg px-3 text-sm outline-none"
            style={{
              border: "1px solid var(--border)",
              background: "var(--bg3)",
              color: "var(--text)",
              fontFamily: "var(--font-app-mono)",
            }}
            autoComplete="off"
          />
          {deviceIdError ? (
            <div className="mt-1 text-xs" style={{ color: "var(--red)", fontFamily: "var(--font-app-mono)" }}>
              {deviceIdError}
            </div>
          ) : null}
        </label>
        <label className="block">
          <div className="text-[11px] font-semibold uppercase tracking-wide" style={{ color: "var(--muted)", fontFamily: "var(--font-app-mono)" }}>
            Range
          </div>
          <select
            value={range}
            onChange={(e) => setRange(e.target.value as KpiRangePreset)}
            className="mt-1 h-10 w-full rounded-lg px-3 text-sm outline-none"
            style={{
              border: "1px solid var(--border)",
              background: "var(--bg3)",
              color: "var(--text)",
              fontFamily: "var(--font-app-mono)",
            }}
          >
            {rangePresets.map((p) => (
              <option key={p.value} value={p.value}>
                {p.label}
              </option>
            ))}
          </select>
        </label>
        <div className="md:pt-5">
          <button
            type="button"
            onClick={() => void onRefresh()}
            disabled={!canRefresh}
            className="h-10 w-full rounded-lg text-sm font-bold disabled:cursor-not-allowed disabled:opacity-50"
            style={{ background: "var(--accent)", color: "#0b0f14", fontFamily: "var(--font-app-mono)" }}
          >
            {loading ? "Loading…" : "Refresh"}
          </button>
        </div>
      </div>

      <div
        className="mx-auto max-w-[1200px] border-b px-4 py-1.5 text-[10px] md:px-7"
        style={{ borderColor: "var(--border)", color: "var(--muted)", fontFamily: "var(--font-app-mono)" }}
      >
        <span style={{ color: "var(--accent)" }}>InfluxDB</span>: snapshot{" "}
        <code className="rounded bg-white/5 px-1">/api/kpi/latest</code> (range {range}) +{" "}
        <code className="rounded bg-white/5 px-1">/api/kpi/hourly?range={range}</code> (same window, bucket {chargingHourly?.aggregateEvery ?? "—"}). Analytics uses Recharts (axes + tooltips); sparklines still fall back to last {chartWindow} poll samples if a series is short.
      </div>

      <div className="mx-auto max-w-[1200px] space-y-6 px-4 py-6 md:px-7">
        {error ? (
          <div
            className="flex flex-col gap-3 rounded-2xl border px-5 py-4"
            style={{ borderColor: "rgba(255,94,94,0.35)", background: "rgba(255,94,94,0.08)", color: "var(--text)" }}
          >
            <div className="flex flex-col gap-2 sm:flex-row sm:items-center sm:justify-between">
              <div className="text-sm font-bold" style={{ fontFamily: "var(--font-app-mono)" }}>
                Failed to load KPIs
              </div>
              <button
                type="button"
                onClick={() => void onRefresh()}
                className="self-start text-sm font-bold underline underline-offset-4 sm:self-auto"
                style={{ color: "var(--accent)" }}
              >
                Retry
              </button>
            </div>
            <div className="text-sm" style={{ color: "var(--muted)" }}>
              {error}
            </div>
          </div>
        ) : null}

        {isEmpty && !loading ? (
          <div className="rounded-2xl border px-5 py-4" style={{ borderColor: "var(--border)", background: "var(--bg2)", color: "var(--text)" }}>
            <div className="text-lg font-bold" style={{ fontFamily: "var(--font-app-mono)" }}>
              No data for this device
            </div>
            <p className="mt-1 text-sm leading-6" style={{ color: "var(--muted)" }}>
              No recent points in Influx for this <code className="rounded bg-white/5 px-1.5 py-0.5">device</code> tag. Confirm the device writes measurements{" "}
              <code className="rounded bg-white/5 px-1.5 py-0.5">mppt</code> /{" "}
              <code className="rounded bg-white/5 px-1.5 py-0.5">inverter</code> (bucket from env) and try a wider range.
            </p>
          </div>
        ) : null}

        {tab === "dashboard" ? (
          <div className="space-y-6" style={{ animation: "pv-fade-in 0.3s ease" }}>
            <EnergyFlowPanel pvV={pvV} mpptW={chgW} acW={acW} batV={batV} />

            <div className="grid grid-cols-1 gap-3.5 sm:grid-cols-2 lg:grid-cols-4">
              <StatCard
                label="PV Voltage"
                value={fmt(pvV, 1)}
                unit="V"
                color="var(--accent)"
                history={historyOrSeries(hourlyByField["mppt_pv_voltage"], hist.pv_voltage)}
                glow
                icon="☀️"
              />
              <StatCard
                label="Charging Power"
                value={fmt(chgW, 1)}
                unit="W"
                color="var(--accent)"
                history={historyOrSeries(hourlyByField["mppt_charging_power"], hist.pv_power)}
              />
              <StatCard
                label="Battery Voltage"
                value={fmt(batV, 2)}
                unit="V"
                color="var(--green)"
                history={historyOrSeries(hourlyByField["mppt_battery_voltage"], hist.battery)}
              />
              <StatCard
                label="AC Output"
                value={fmt(acW, 1)}
                unit="W"
                color="var(--cyan)"
                history={historyOrSeries(hourlyByField["inverter_ac_power"], hist.ac_power)}
              />
            </div>

            <div className="grid grid-cols-1 gap-3.5 lg:grid-cols-2">
              <div className="rounded-xl border p-5 md:p-6" style={{ background: "var(--bg2)", borderColor: "var(--border)" }}>
                <div className="mb-3.5 text-[11px] font-bold uppercase tracking-[0.1em]" style={{ fontFamily: "var(--font-app-mono)", color: "var(--muted)" }}>
                  Battery Status
                </div>
                <div className="mb-3">
                  <div className="text-[11px]" style={{ color: "var(--muted)", fontFamily: "var(--font-app-mono)" }}>
                    State of charge
                  </div>
                  <div className="text-[28px] font-bold leading-none" style={{ fontFamily: "var(--font-app-mono)", color: "var(--green)" }}>
                    {batSoc != null && Number.isFinite(batSoc) ? (
                      <>
                        {fmt(batSoc, 0)}
                        <span className="text-sm opacity-60"> %</span>
                      </>
                    ) : (
                      <span className="text-base opacity-60">—</span>
                    )}
                  </div>
                  <div className="mt-2 flex flex-wrap gap-x-3 gap-y-0.5 text-[11px]" style={{ color: "var(--muted)", fontFamily: "var(--font-app-mono)" }}>
                    <span>
                      {batV != null && Number.isFinite(batV) ? `${fmt(batV, 2)} V` : "— V"}
                    </span>
                    <span className="opacity-40">·</span>
                    <span>Charging {chgA != null ? fmt(chgA * 1000, 0) : "—"} mA</span>
                  </div>
                </div>
                <BatterySocBar socPercent={batSoc} />
              </div>

              <div className="rounded-xl border p-5 md:p-6" style={{ background: "var(--bg2)", borderColor: "var(--border)" }}>
                <div className="mb-3.5 text-[11px] font-bold uppercase tracking-[0.1em]" style={{ fontFamily: "var(--font-app-mono)", color: "var(--muted)" }}>
                  Inverter AC
                </div>
                <div className="grid grid-cols-2 gap-2.5">
                  {[
                    { l: "Voltage", v: fmt(acV, 1), u: "V", c: "var(--cyan)" },
                    { l: "Current", v: fmt(acI, 3), u: "A", c: "var(--cyan)" },
                    { l: "Frequency", v: fmt(acHz, 1), u: "Hz", c: "var(--text)" },
                    { l: "Energy", v: fmt(acE, 3), u: "kWh", c: "var(--amber)" },
                  ].map((row) => (
                    <div key={row.l} className="rounded-lg px-3 py-2.5" style={{ background: "var(--bg3)" }}>
                      <div className="mb-1 text-[10px]" style={{ color: "var(--muted)", fontFamily: "var(--font-app-mono)" }}>
                        {row.l}
                      </div>
                      <div className="text-base font-bold" style={{ fontFamily: "var(--font-app-mono)", color: row.c }}>
                        {row.v}
                        <span className="text-[10px] opacity-60"> {row.u}</span>
                      </div>
                    </div>
                  ))}
                </div>
              </div>
            </div>
          </div>
        ) : null}

        {tab === "analytics" ? (
          <div className="space-y-4" style={{ animation: "pv-fade-in 0.3s ease" }}>
            <AnalyticsPowerCompare
              range={range}
              charging={hourlyByField["mppt_charging_power"]}
              ac={hourlyByField["inverter_ac_power"]}
              histChg={hist.pv_power}
              histAc={hist.ac_power}
            />

            <div className="grid grid-cols-1 gap-4 lg:grid-cols-2">
              <AnalyticsAreaPanel
                title="PV Charging Power"
                unit="W"
                color="var(--accent)"
                variant="power"
                series={hourlyByField["mppt_charging_power"]}
                fallback={hist.pv_power}
                range={range}
                stat={`${fmt(chgW, 1)} W`}
                statHint="Latest snapshot"
                foot={
                  seriesValuesForChart(hourlyByField["mppt_charging_power"]).length > 1
                    ? `Influx · −${hourlyByField["mppt_charging_power"]?.range ?? range} · every ${hourlyByField["mppt_charging_power"]?.aggregateEvery ?? "—"}`
                    : `Poll buffer · last ${chartWindow} samples`
                }
              />
              <AnalyticsAreaPanel
                title="AC Output Power"
                unit="W"
                color="var(--cyan)"
                variant="power"
                series={hourlyByField["inverter_ac_power"]}
                fallback={hist.ac_power}
                range={range}
                stat={`${fmt(acW, 1)} W`}
                statHint="Latest snapshot"
                foot={
                  seriesValuesForChart(hourlyByField["inverter_ac_power"]).length > 1
                    ? `Influx · −${hourlyByField["inverter_ac_power"]?.range ?? range} · every ${hourlyByField["inverter_ac_power"]?.aggregateEvery ?? "—"}`
                    : `Poll buffer · last ${chartWindow} samples`
                }
              />
              <AnalyticsAreaPanel
                title="Battery Voltage Trend"
                unit="V"
                color="var(--green)"
                variant="voltage"
                series={hourlyByField["mppt_battery_voltage"]}
                fallback={hist.battery}
                range={range}
                stat={`${fmt(batV, 2)} V`}
                statHint="Latest snapshot"
                foot={
                  seriesValuesForChart(hourlyByField["mppt_battery_voltage"]).length > 1
                    ? `Influx · −${hourlyByField["mppt_battery_voltage"]?.range ?? range} · every ${hourlyByField["mppt_battery_voltage"]?.aggregateEvery ?? "—"}`
                    : `Poll buffer · last ${chartWindow} samples`
                }
              />
              <AnalyticsAreaPanel
                title="Grid Frequency"
                unit="Hz"
                color="#c8a2ff"
                variant="frequency"
                series={hourlyByField["inverter_ac_frequency"]}
                fallback={hist.ac_freq}
                range={range}
                stat={`${fmt(acHz, 2)} Hz`}
                statHint="Nominal 50 Hz"
                foot={
                  seriesValuesForChart(hourlyByField["inverter_ac_frequency"]).length > 1
                    ? `Influx · −${hourlyByField["inverter_ac_frequency"]?.range ?? range} · every ${hourlyByField["inverter_ac_frequency"]?.aggregateEvery ?? "—"}`
                    : `Poll buffer · last ${chartWindow} samples`
                }
              />
            </div>

            <AnalyticsChargingEnergyBar
              series={chargingHourly}
              range={range}
              kwhTotal={kwhFromInfluxHourly}
              peakWibHour={peakWibHour}
              peakMeanW={hourlyValues.length ? `${fmt(Math.max(...hourlyValues, 0), 1)} W` : "—"}
            />
          </div>
        ) : null}

        {tab === "inverter" ? (
          <div className="grid grid-cols-1 gap-4 lg:grid-cols-2" style={{ animation: "pv-fade-in 0.3s ease" }}>
            <div className="rounded-xl border p-6" style={{ background: "var(--bg2)", borderColor: "var(--border)" }}>
              <div className="mb-5 text-[11px] font-bold uppercase tracking-[0.1em]" style={{ fontFamily: "var(--font-app-mono)", color: "var(--muted)" }}>
                AC Output Parameters
              </div>
              {[
                { label: "AC Voltage", value: fmt(acV, 1), unit: "V", pct: (acV ?? 0) / 240, color: "var(--cyan)" },
                { label: "AC Current", value: fmt(acI, 3), unit: "A", pct: (acI ?? 0) / 10, color: "var(--cyan)" },
                { label: "Active Power", value: fmt(acW, 1), unit: "W", pct: (acW ?? 0) / 500, color: "var(--accent)" },
                { label: "Apparent Power", value: fmt(apparent, 1), unit: "VA", pct: (apparent ?? 0) / 500, color: "var(--accent)" },
                { label: "Frequency", value: fmt(acHz, 2), unit: "Hz", pct: ((acHz ?? 50) - 48) / 4, color: "#c8a2ff" },
                { label: "Power Factor", value: fmt(pf, 3), unit: "", pct: pf ?? 0, color: "var(--green)" },
              ].map((row) => (
                <div key={row.label} className="mb-4">
                  <div className="mb-1.5 flex justify-between">
                    <span className="text-[11px]" style={{ color: "var(--muted)", fontFamily: "var(--font-app-mono)" }}>
                      {row.label}
                    </span>
                    <span className="text-[13px] font-bold" style={{ fontFamily: "var(--font-app-mono)", color: row.color }}>
                      {row.value}
                      <span className="text-[10px] opacity-60"> {row.unit}</span>
                    </span>
                  </div>
                  <div className="h-1 rounded" style={{ background: "rgba(255,255,255,0.05)" }}>
                    <div
                      className="h-full rounded transition-[width]"
                      style={{
                        width: `${clamp(row.pct, 0, 1) * 100}%`,
                        background: row.color,
                        boxShadow: `0 0 6px color-mix(in srgb, ${row.color} 40%, transparent)`,
                      }}
                    />
                  </div>
                </div>
              ))}
            </div>

            <div className="space-y-4">
              <div className="rounded-xl border p-6" style={{ background: "var(--bg2)", borderColor: "var(--border)" }}>
                <div className="mb-5 text-[11px] font-bold uppercase tracking-[0.1em]" style={{ fontFamily: "var(--font-app-mono)", color: "var(--muted)" }}>
                  MPPT Controller
                </div>
                {[
                  { label: "PV Voltage", value: fmt(pvV, 1), unit: "V", color: "var(--accent)" },
                  { label: "Charging Power", value: fmt(chgW, 1), unit: "W", color: "var(--accent)" },
                  { label: "Charging Current", value: chgA != null ? fmt(chgA * 1000, 0) : "—", unit: "mA", color: "var(--accent)" },
                  { label: "Battery Voltage", value: fmt(batV, 2), unit: "V", color: "var(--green)" },
                  { label: "Load Current", value: fmt(s.mppt_load_current, 3), unit: "A", color: "var(--muted)" },
                  { label: "Load Power", value: fmt(loadW, 1), unit: "W", color: "var(--muted)" },
                ].map((row) => (
                  <div
                    key={row.label}
                    className="flex items-center justify-between border-b py-2 last:border-b-0"
                    style={{ borderColor: "var(--border)" }}
                  >
                    <span className="text-[11px]" style={{ color: "var(--muted)", fontFamily: "var(--font-app-mono)" }}>
                      {row.label}
                    </span>
                    <span className="text-sm font-bold" style={{ fontFamily: "var(--font-app-mono)", color: row.color }}>
                      {row.value}
                      <span className="text-[10px] opacity-60"> {row.unit}</span>
                    </span>
                  </div>
                ))}
              </div>

              <div className="rounded-xl border p-6" style={{ background: "var(--bg2)", borderColor: "var(--border)" }}>
                <div className="mb-3 text-[11px] font-bold uppercase tracking-[0.1em]" style={{ fontFamily: "var(--font-app-mono)", color: "var(--muted)" }}>
                  Total Energy
                </div>
                <div className="text-[40px] font-bold leading-none" style={{ fontFamily: "var(--font-app-mono)", color: "var(--accent)" }}>
                  {fmt(acE, 3)}
                </div>
                <div className="mt-1 text-xs" style={{ color: "var(--muted)" }}>
                  kWh accumulated
                </div>
                <div className="mt-4 flex gap-2.5">
                  <div className="flex-1 rounded-lg px-3 py-2.5 text-center" style={{ background: "var(--bg3)" }}>
                    <div className="mb-1 text-[9px]" style={{ color: "var(--muted)", fontFamily: "var(--font-app-mono)" }}>
                      WINDOW ({(chargingHourly?.range ?? range).toUpperCase()} · INFLUX)
                    </div>
                    <div className="text-base font-bold" style={{ fontFamily: "var(--font-app-mono)", color: "var(--accent)" }}>
                      {kwhFromInfluxHourly != null ? `${fmt(kwhFromInfluxHourly, 1)} kWh` : "—"}
                    </div>
                  </div>
                  <div className="flex-1 rounded-lg px-3 py-2.5 text-center" style={{ background: "var(--bg3)" }}>
                    <div className="mb-1 text-[9px]" style={{ color: "var(--muted)", fontFamily: "var(--font-app-mono)" }}>
                      INVERTER ENERGY (SNAPSHOT)
                    </div>
                    <div className="text-base font-bold" style={{ fontFamily: "var(--font-app-mono)", color: "var(--green)" }}>
                      {fmt(acE, 1)} kWh
                    </div>
                  </div>
                </div>
              </div>
            </div>
          </div>
        ) : null}

        {tab === "system" ? (
          <div className="grid grid-cols-1 gap-4 lg:grid-cols-2" style={{ animation: "pv-fade-in 0.3s ease" }}>
            <div className="rounded-xl border p-6" style={{ background: "var(--bg2)", borderColor: "var(--border)" }}>
              <div className="mb-5 text-[11px] font-bold uppercase tracking-[0.1em]" style={{ fontFamily: "var(--font-app-mono)", color: "var(--muted)" }}>
                Device Info
              </div>
              {[
                { k: "Device ID", v: deviceId.trim() || "—" },
                { k: "MPPT Status", v: mpptCharging ? "Charging ✓" : "Idle", c: "var(--green)" },
                { k: "Inverter Status", v: (s.inverter_valid ?? 0) > 0 ? "Valid ✓" : "—", c: "var(--green)" },
                { k: "Charging Mode", v: mpptCharging ? "MPPT Charging" : "Standby" },
                {
                  k: "Fault Code",
                  v: fault != null ? `0x${Math.max(0, Math.floor(fault)).toString(16).padStart(4, "0")}` : "—",
                  c: fault != null && fault !== 0 ? "var(--amber)" : undefined,
                },
                { k: "Data range", v: range },
                { k: "Last asOf", v: formatAsOfUtc(data?.asOf) },
              ].map((row) => (
                <div key={row.k} className="flex justify-between border-b py-2.5 last:border-b-0" style={{ borderColor: "var(--border)" }}>
                  <span className="text-[11px]" style={{ color: "var(--muted)", fontFamily: "var(--font-app-mono)" }}>
                    {row.k}
                  </span>
                  <span className="text-xs font-bold" style={{ fontFamily: "var(--font-app-mono)", color: row.c ?? "var(--text)" }}>
                    {row.v}
                  </span>
                </div>
              ))}
            </div>

            <div className="space-y-4">
              <div className="rounded-xl border p-6" style={{ background: "var(--bg2)", borderColor: "var(--border)" }}>
                <div className="mb-4 text-[11px] font-bold uppercase tracking-[0.1em]" style={{ fontFamily: "var(--font-app-mono)", color: "var(--muted)" }}>
                  Alerts
                </div>
                {fault != null && fault !== 0 ? (
                  <div
                    className="mb-2.5 flex items-center gap-3 rounded-lg border px-3.5 py-3"
                    style={{ background: "rgba(245,166,35,0.08)", borderColor: "rgba(245,166,35,0.2)" }}
                  >
                    <span className="text-base">⚠️</span>
                    <div>
                      <div className="text-xs font-bold" style={{ color: "var(--amber)" }}>
                        Fault Code 0x{Math.max(0, Math.floor(fault)).toString(16)}
                      </div>
                      <div className="text-[10px]" style={{ color: "var(--muted)" }}>
                        Check device documentation for this fault bitmask.
                      </div>
                    </div>
                  </div>
                ) : null}
                <div
                  className="flex items-center gap-3 rounded-lg border px-3.5 py-3"
                  style={{ background: "var(--green-dim)", borderColor: "rgba(57,217,138,0.2)" }}
                >
                  <span className="text-base">✅</span>
                  <div>
                    <div className="text-xs font-bold" style={{ color: "var(--green)" }}>
                      All Systems Nominal
                    </div>
                    <div className="text-[10px]" style={{ color: "var(--muted)" }}>
                      MPPT and inverter snapshot loaded from Influx.
                    </div>
                  </div>
                </div>
              </div>

              <div className="rounded-xl border p-4" style={{ background: "var(--bg2)", borderColor: "var(--border)" }}>
                <div className="mb-2 text-[10px] font-bold uppercase tracking-wide" style={{ color: "var(--muted)", fontFamily: "var(--font-app-mono)" }}>
                  Spark preview
                </div>
                <div className="grid grid-cols-2 gap-2">
                  <div className="rounded-lg border p-2" style={{ borderColor: "var(--border)" }}>
                    <div className="text-[9px]" style={{ color: "var(--muted)", fontFamily: "var(--font-app-mono)" }}>
                      PV V
                    </div>
                    {(() => {
                      const d = historyOrSeries(hourlyByField["mppt_pv_voltage"], hist.pv_voltage);
                      return d ? <Sparkline data={d} color="var(--accent)" height={32} /> : null;
                    })()}
                  </div>
                  <div className="rounded-lg border p-2" style={{ borderColor: "var(--border)" }}>
                    <div className="text-[9px]" style={{ color: "var(--muted)", fontFamily: "var(--font-app-mono)" }}>
                      AC W
                    </div>
                    {(() => {
                      const d = historyOrSeries(hourlyByField["inverter_ac_power"], hist.ac_power);
                      return d ? <Sparkline data={d} color="var(--cyan)" height={32} /> : null;
                    })()}
                  </div>
                </div>
              </div>
            </div>
          </div>
        ) : null}
      </div>
    </div>
  );
}

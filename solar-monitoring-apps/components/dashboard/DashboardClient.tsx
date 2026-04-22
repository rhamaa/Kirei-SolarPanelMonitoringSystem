"use client";

import { useCallback, useMemo, useState } from "react";

import type { KpiRangePreset } from "@/lib/influx/queries";
import type { LatestKpiResponse } from "@/lib/kpi/types";

import { KpiGrid, type KpiGridItem } from "@/components/kpi/KpiGrid";

const DEVICE_ID_RE = /^[A-Za-z0-9:_-]+$/;

const RANGE_PRESETS: Array<{ value: KpiRangePreset; label: string }> = [
  { value: "1h", label: "1h" },
  { value: "6h", label: "6h" },
  { value: "24h", label: "24h" },
  { value: "7d", label: "7d" },
];

const KPI_LABELS: Record<string, string> = {
  pv_voltage: "PV Voltage",
  pv_current: "PV Current",
  pv_power: "PV Power",
  battery_voltage: "Battery Voltage",
  inverter_power: "Inverter Power",
};

export function DashboardClient(props: {
  defaultDeviceId: string;
  defaultRange: KpiRangePreset;
}) {
  const { defaultDeviceId, defaultRange } = props;

  const [deviceId, setDeviceId] = useState(defaultDeviceId);
  const [range, setRange] = useState<KpiRangePreset>(defaultRange ?? "24h");
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [data, setData] = useState<LatestKpiResponse | null>(null);

  const deviceIdError = useMemo(() => {
    const trimmed = deviceId.trim();
    if (!trimmed) return "Device ID is required.";
    if (trimmed.length > 128) return "Device ID is too long.";
    if (!DEVICE_ID_RE.test(trimmed)) return "Use only letters, numbers, :, _, and -.";
    return null;
  }, [deviceId]);

  const canRefresh = !deviceIdError && !loading;

  const fetchKpis = useCallback(async () => {
    const trimmed = deviceId.trim();
    if (!trimmed || deviceIdError) return;

    setLoading(true);
    setError(null);

    try {
      const url = new URL("/api/kpi/latest", window.location.origin);
      url.searchParams.set("device_id", trimmed);
      url.searchParams.set("range", range);

      const res = await fetch(url.toString(), { cache: "no-store" });
      const json = (await res.json().catch(() => null)) as unknown;

      if (!res.ok) {
        const message =
          typeof json === "object" && json && "message" in json
            ? String((json as { message?: unknown }).message ?? "Failed to load KPIs")
            : "Failed to load KPIs";
        throw new Error(message);
      }

      setData(json as LatestKpiResponse);
    } catch (e) {
      setData(null);
      setError(e instanceof Error ? e.message : "Failed to load KPIs");
    } finally {
      setLoading(false);
    }
  }, [deviceId, deviceIdError, range]);

  const isEmpty = useMemo(() => {
    if (!data) return false;
    const values = Object.values(data.values ?? {});
    return values.length > 0 && values.every((v) => v.value === null);
  }, [data]);

  const kpiItems: KpiGridItem[] = useMemo(() => {
    const keys = Object.keys(KPI_LABELS);
    if (loading || !data) {
      return keys.map((key) => ({ key, label: KPI_LABELS[key] ?? key, state: "loading" }));
    }

    return keys.map((key) => ({
      key,
      label: KPI_LABELS[key] ?? key,
      state: "ready",
      value: data.values?.[key]?.value ?? null,
      unit: data.values?.[key]?.unit ?? "",
      time: data.values?.[key]?.time ?? null,
    }));
  }, [data, loading]);

  return (
    <div className="space-y-6">
      <div className="sticky top-0 z-10 -mx-4 border-b border-black/5 bg-background/80 px-4 py-4 backdrop-blur dark:border-white/10">
        <div className="flex flex-col gap-3 sm:flex-row sm:items-start sm:justify-between">
          <div>
            <h1 className="text-xl font-semibold tracking-tight text-zinc-950 dark:text-zinc-50">
              Dashboard
            </h1>
            <div className="mt-1 text-sm text-zinc-600 dark:text-zinc-400">
              Single device · Phase 1 shell
            </div>
          </div>

          <div className="w-full max-w-xl rounded-2xl bg-zinc-100 p-3 dark:bg-zinc-900">
            <div className="grid grid-cols-1 gap-3 sm:grid-cols-[1fr_120px_140px] sm:items-start">
              <label className="block">
                <div className="text-sm font-semibold text-zinc-900 dark:text-zinc-100">
                  Device ID
                </div>
                <input
                  type="text"
                  value={deviceId}
                  onChange={(e) => setDeviceId(e.target.value)}
                  className="mt-1 h-11 w-full rounded-xl border border-black/10 bg-white px-3 text-sm text-zinc-950 outline-none ring-0 focus:border-green-600 focus:ring-2 focus:ring-green-600/20 dark:border-white/10 dark:bg-zinc-950 dark:text-zinc-50"
                  placeholder="e.g. dev:inverter-01"
                  autoComplete="off"
                  inputMode="text"
                />
                {deviceIdError ? (
                  <div className="mt-1 text-sm text-red-600 dark:text-red-400">
                    {deviceIdError}
                  </div>
                ) : null}
              </label>

              <label className="block">
                <div className="text-sm font-semibold text-zinc-900 dark:text-zinc-100">
                  Range
                </div>
                <select
                  value={range}
                  onChange={(e) => setRange(e.target.value as KpiRangePreset)}
                  className="mt-1 h-11 w-full rounded-xl border border-black/10 bg-white px-3 text-sm text-zinc-950 outline-none focus:border-green-600 focus:ring-2 focus:ring-green-600/20 dark:border-white/10 dark:bg-zinc-950 dark:text-zinc-50"
                >
                  {RANGE_PRESETS.map((p) => (
                    <option key={p.value} value={p.value}>
                      {p.label}
                    </option>
                  ))}
                </select>
                <div className="mt-1 text-sm text-zinc-600 dark:text-zinc-400">
                  Custom range is coming in Phase 2.
                </div>
              </label>

              <div className="sm:pt-6">
                <button
                  type="button"
                  onClick={() => void fetchKpis()}
                  disabled={!canRefresh}
                  className="h-11 w-full rounded-xl bg-green-600 px-4 text-sm font-semibold text-white shadow-sm disabled:cursor-not-allowed disabled:opacity-50"
                >
                  Refresh KPIs
                </button>
              </div>
            </div>
          </div>
        </div>
      </div>

      {error ? (
        <div className="rounded-2xl border border-red-200 bg-red-50 px-5 py-4 text-red-950 dark:border-red-900/50 dark:bg-red-950/25 dark:text-red-100">
          <div className="flex flex-col gap-2 sm:flex-row sm:items-center sm:justify-between">
            <div className="text-sm font-semibold">Failed to load KPIs</div>
            <button
              type="button"
              onClick={() => void fetchKpis()}
              className="text-sm font-semibold text-red-700 underline underline-offset-4 hover:text-red-800 dark:text-red-200 dark:hover:text-red-100"
            >
              Retry
            </button>
          </div>
          <div className="mt-1 text-sm text-red-700 dark:text-red-200">{error}</div>
        </div>
      ) : null}

      {isEmpty && !loading ? (
        <div className="rounded-2xl border border-black/5 bg-zinc-50 px-5 py-4 dark:border-white/10 dark:bg-zinc-950">
          <div className="text-lg font-semibold leading-6 text-zinc-950 dark:text-zinc-50">
            No data for this device
          </div>
          <p className="mt-1 text-sm leading-6 text-zinc-700 dark:text-zinc-300">
            We couldn’t find any recent points in Influx for this{" "}
            <code className="rounded bg-black/5 px-1.5 py-0.5 text-[0.95em] dark:bg-white/10">
              device_id
            </code>
            . Verify the device is writing to measurement{" "}
            <code className="rounded bg-black/5 px-1.5 py-0.5 text-[0.95em] dark:bg-white/10">
              pv_monitoring
            </code>{" "}
            and try a wider time range.
          </p>
        </div>
      ) : null}

      <KpiGrid items={kpiItems} />
    </div>
  );
}


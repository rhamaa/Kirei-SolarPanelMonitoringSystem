"use client";

import { useCallback, useEffect, useMemo, useRef, useState } from "react";

import type { KpiRangePreset } from "@/lib/influx/queries";
import { INFLUX_CHART_FIELDS, type InfluxChartField } from "@/lib/influx/hourly-fields";
import type { HourlySeriesResponse, LatestKpiResponse } from "@/lib/kpi/types";

import { PvMonitorChrome, type PvHistory } from "@/components/dashboard/pv-monitor/pv-monitor-chrome";

const DEVICE_ID_RE = /^[A-Za-z0-9:_-]+$/;

const RANGE_PRESETS: Array<{ value: KpiRangePreset; label: string }> = [
  { value: "1h", label: "1h" },
  { value: "6h", label: "6h" },
  { value: "24h", label: "24h" },
  { value: "7d", label: "7d" },
];

const CHART_WINDOW = 50;
const POLL_MS = 8000;

function pushRing(arr: number[], v: number, max: number): number[] {
  return [...arr, v].slice(-max);
}

export function DashboardClient(props: { defaultDeviceId: string; defaultRange: KpiRangePreset }) {
  const { defaultDeviceId, defaultRange } = props;

  const [deviceId, setDeviceId] = useState(defaultDeviceId);
  const [range, setRange] = useState<KpiRangePreset>(defaultRange ?? "24h");
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [data, setData] = useState<LatestKpiResponse | null>(null);
  const [hourlyByField, setHourlyByField] = useState<Partial<Record<InfluxChartField, HourlySeriesResponse>>>({});

  const [hist, setHist] = useState<PvHistory>({
    pv_voltage: [],
    pv_power: [],
    battery: [],
    ac_power: [],
    ac_freq: [],
  });

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
      const urlLatest = new URL("/api/kpi/latest", window.location.origin);
      urlLatest.searchParams.set("device_id", trimmed);
      urlLatest.searchParams.set("range", range);

      const hourlyUrls = INFLUX_CHART_FIELDS.map((field) => {
        const urlHourly = new URL("/api/kpi/hourly", window.location.origin);
        urlHourly.searchParams.set("device_id", trimmed);
        urlHourly.searchParams.set("field", field);
        urlHourly.searchParams.set("range", range);
        return urlHourly.toString();
      });

      const allResponses = await Promise.all([
        fetch(urlLatest.toString(), { cache: "no-store" }),
        ...hourlyUrls.map((u) => fetch(u, { cache: "no-store" })),
      ]);

      const resLatest = allResponses[0];
      const hourlyResponses = allResponses.slice(1);

      const hourlyResults = await Promise.all(
        INFLUX_CHART_FIELDS.map(async (field, i) => {
          const resHourly = hourlyResponses[i];
          if (!resHourly?.ok) return [field, null] as const;
          const jsonHourly = (await resHourly.json().catch(() => null)) as unknown;
          if (
            jsonHourly &&
            typeof jsonHourly === "object" &&
            "points" in jsonHourly &&
            Array.isArray((jsonHourly as HourlySeriesResponse).points)
          ) {
            return [field, jsonHourly as HourlySeriesResponse] as const;
          }
          return [field, null] as const;
        }),
      );

      const jsonLatest = (await resLatest.json().catch(() => null)) as unknown;

      if (!resLatest.ok) {
        const message =
          typeof jsonLatest === "object" && jsonLatest && "message" in jsonLatest
            ? String((jsonLatest as { message?: unknown }).message ?? "Failed to load KPIs")
            : "Failed to load KPIs";
        throw new Error(message);
      }

      setData(jsonLatest as LatestKpiResponse);

      const nextHourly: Partial<Record<InfluxChartField, HourlySeriesResponse>> = {};
      for (const [field, body] of hourlyResults) {
        if (body) nextHourly[field] = body;
      }
      setHourlyByField(nextHourly);
    } catch (e) {
      setData(null);
      setHourlyByField({});
      setError(e instanceof Error ? e.message : "Failed to load KPIs");
    } finally {
      setLoading(false);
    }
  }, [deviceId, deviceIdError, range]);

  const fetchKpisRef = useRef(fetchKpis);
  fetchKpisRef.current = fetchKpis;

  useEffect(() => {
    void fetchKpisRef.current();
  }, [range, deviceId]);

  useEffect(() => {
    const id = window.setInterval(() => void fetchKpisRef.current(), POLL_MS);
    return () => window.clearInterval(id);
  }, []);

  useEffect(() => {
    if (!data?.snapshot) return;
    const s = data.snapshot;
    setHist((h) => ({
      pv_voltage: s.mppt_pv_voltage != null ? pushRing(h.pv_voltage, s.mppt_pv_voltage, CHART_WINDOW) : h.pv_voltage,
      pv_power: s.mppt_charging_power != null ? pushRing(h.pv_power, s.mppt_charging_power, CHART_WINDOW) : h.pv_power,
      battery: s.mppt_battery_voltage != null ? pushRing(h.battery, s.mppt_battery_voltage, CHART_WINDOW) : h.battery,
      ac_power: s.inverter_ac_power != null ? pushRing(h.ac_power, s.inverter_ac_power, CHART_WINDOW) : h.ac_power,
      ac_freq: s.inverter_ac_frequency != null ? pushRing(h.ac_freq, s.inverter_ac_frequency, CHART_WINDOW) : h.ac_freq,
    }));
  }, [data]);

  const isEmpty = useMemo(() => {
    if (!data) return false;
    const values = Object.values(data.values ?? {});
    return values.length > 0 && values.every((v) => v.value === null);
  }, [data]);

  return (
    <PvMonitorChrome
      deviceId={deviceId}
      setDeviceId={setDeviceId}
      deviceIdError={deviceIdError}
      range={range}
      setRange={setRange}
      rangePresets={RANGE_PRESETS}
      onRefresh={fetchKpis}
      canRefresh={canRefresh}
      loading={loading}
      error={error}
      data={data}
      hourlyByField={hourlyByField}
      isEmpty={isEmpty}
      hist={hist}
      chartWindow={CHART_WINDOW}
      updateIntervalMs={POLL_MS}
    />
  );
}

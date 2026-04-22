import { NextResponse } from "next/server";
import { z } from "zod";

import { INFLUX_CHART_FIELDS } from "@/lib/influx/hourly-fields";
import { getQueryApi } from "@/lib/influx/client";
import {
  aggregateEveryForRange,
  buildHourlyMeanFlux,
  parseFluxEveryToHours,
  type KpiRangePreset,
} from "@/lib/influx/queries";
import { normalizeHourlyMeanSeries } from "@/lib/influx/normalize-hourly";

export const runtime = "nodejs";

const DeviceIdSchema = z
  .string()
  .min(1)
  .max(128)
  .regex(/^[A-Za-z0-9:_-]+$/);

const FieldSchema = z.enum(INFLUX_CHART_FIELDS);

const RangeSchema = z.enum(["1h", "6h", "24h", "7d"]);

const FORBIDDEN_QUERY_KEYS = new Set([
  "url",
  "org",
  "bucket",
  "token",
  "influx_url",
  "influx_org",
  "influx_bucket",
  "influx_token",
]);

function json(status: number, body: unknown, headers?: HeadersInit) {
  return NextResponse.json(body, {
    status,
    headers: {
      "Cache-Control": "no-store",
      ...(headers ?? {}),
    },
  });
}

export async function GET(request: Request) {
  const url = new URL(request.url);
  const params = url.searchParams;

  for (const key of params.keys()) {
    if (FORBIDDEN_QUERY_KEYS.has(key.toLowerCase())) {
      return json(400, { error: "invalid_query", message: `Forbidden query key: ${key}` });
    }
  }

  let env: {
    INFLUX_BUCKET: string;
    DEFAULT_DEVICE_ID: string;
    KPI_RANGE_DEFAULT: KpiRangePreset;
  };

  try {
    ({ env } = await import("@/lib/env"));
  } catch (e) {
    const message = e instanceof Error ? e.message : "Invalid server environment";
    return json(500, { error: "env_invalid", message });
  }

  const deviceIdRaw = params.get("device_id") ?? env.DEFAULT_DEVICE_ID;
  const fieldRaw = params.get("field") ?? "mppt_charging_power";
  const rangeRaw = params.get("range") ?? env.KPI_RANGE_DEFAULT;

  const parsed = z
    .object({
      deviceId: DeviceIdSchema,
      field: FieldSchema,
      range: RangeSchema,
    })
    .safeParse({ deviceId: deviceIdRaw, field: fieldRaw, range: rangeRaw });

  if (!parsed.success) {
    return json(400, { error: "invalid_query", message: "Invalid query parameters" });
  }

  const { deviceId, field, range } = parsed.data;
  const flux = buildHourlyMeanFlux({ bucket: env.INFLUX_BUCKET, deviceId, field, range });
  const aggregateEvery = aggregateEveryForRange(range);
  const bucketDurationHours = parseFluxEveryToHours(aggregateEvery);

  try {
    const queryApi = await getQueryApi();
    const rows = (await queryApi.collectRows(flux)) as Array<Record<string, unknown>>;
    const normalized = normalizeHourlyMeanSeries({ deviceId, field, rows, asOf: new Date() });
    return json(200, {
      ...normalized,
      range,
      aggregateEvery,
      bucketDurationHours,
    });
  } catch {
    return json(502, { error: "influx_unavailable" });
  }
}

import { NextResponse } from "next/server";
import { z } from "zod";

import { getQueryApi } from "@/lib/influx/client";
import { mergeLatestFieldRowsToCanonicalRow } from "@/lib/influx/influx-schema";
import { buildLatestInverterLastFieldsFlux, buildLatestMpptLastFieldsFlux, type KpiRangePreset } from "@/lib/influx/queries";
import { normalizeLatestKpi } from "@/lib/influx/normalize";

export const runtime = "nodejs";

const RangeSchema = z.enum(["1h", "6h", "24h", "7d"]).default("24h");
const DeviceIdSchema = z
  .string()
  .min(1)
  .max(128)
  .regex(/^[A-Za-z0-9:_-]+$/);

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
  const searchParams = url.searchParams;

  for (const key of searchParams.keys()) {
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

  const deviceIdRaw = searchParams.get("device_id") ?? env.DEFAULT_DEVICE_ID;
  const rangeRaw = searchParams.get("range") ?? env.KPI_RANGE_DEFAULT;

  const parsed = z
    .object({
      deviceId: DeviceIdSchema,
      range: RangeSchema,
    })
    .safeParse({ deviceId: deviceIdRaw, range: rangeRaw });

  if (!parsed.success) {
    return json(400, { error: "invalid_query", message: "Invalid query parameters" });
  }

  const { deviceId, range } = parsed.data;
  const latestArgs = { bucket: env.INFLUX_BUCKET, deviceId, range } as const;
  const mpptFlux = buildLatestMpptLastFieldsFlux(latestArgs);
  const inverterFlux = buildLatestInverterLastFieldsFlux(latestArgs);

  try {
    const queryApi = await getQueryApi();
    const [mpptRows, inverterRows] = await Promise.all([
      queryApi.collectRows(mpptFlux) as Promise<Array<Record<string, unknown>>>,
      queryApi.collectRows(inverterFlux) as Promise<Array<Record<string, unknown>>>,
    ]);
    const merged = mergeLatestFieldRowsToCanonicalRow(mpptRows, inverterRows);
    const body = normalizeLatestKpi({ deviceId, range, rows: [merged], asOf: new Date() });
    return json(200, body);
  } catch {
    return json(502, { error: "influx_unavailable" });
  }
}

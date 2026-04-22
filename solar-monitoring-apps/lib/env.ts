import "server-only";

import { createEnv } from "@t3-oss/env-nextjs";
import { z } from "zod";

function formatEnvError(varNames: string[]) {
  const unique = Array.from(new Set(varNames)).sort();
  return `Invalid environment variables: ${unique.join(", ")}`;
}

export const env = createEnv({
  server: {
    INFLUX_URL: z.string().url(),
    INFLUX_ORG: z.string().min(1),
    INFLUX_BUCKET: z.string().min(1),
    INFLUX_TOKEN: z.string().min(1),
    DEFAULT_DEVICE_ID: z.string().min(1),
    KPI_RANGE_DEFAULT: z.enum(["1h", "6h", "24h", "7d"]).default("24h"),
    KPI_REFRESH_MS: z.coerce.number().int().min(1000).default(10_000),
  },
  client: {},
  runtimeEnv: {
    INFLUX_URL: process.env.INFLUX_URL,
    INFLUX_ORG: process.env.INFLUX_ORG,
    INFLUX_BUCKET: process.env.INFLUX_BUCKET,
    INFLUX_TOKEN: process.env.INFLUX_TOKEN,
    DEFAULT_DEVICE_ID: process.env.DEFAULT_DEVICE_ID,
    KPI_RANGE_DEFAULT: process.env.KPI_RANGE_DEFAULT,
    KPI_REFRESH_MS: process.env.KPI_REFRESH_MS,
  },
  emptyStringAsUndefined: true,
  onValidationError: (error) => {
    const maybeIssues =
      (error as unknown as { issues?: Array<{ path?: unknown[] }> }).issues ??
      (error as unknown as { errors?: Array<{ path?: unknown[] }> }).errors ??
      (Array.isArray(error) ? (error as Array<{ path?: unknown[] }>) : []);

    const varNames = maybeIssues
      .map((issue) => String(issue?.path?.[0] ?? "unknown"))
      .filter((x) => x !== "unknown");
    throw new Error(formatEnvError(varNames.length ? varNames : ["unknown"]));
  },
});


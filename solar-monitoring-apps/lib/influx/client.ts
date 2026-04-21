import "server-only";

import { InfluxDB } from "@influxdata/influxdb-client";

export async function getQueryApi() {
  const { env } = await import("@/lib/env");
  const influx = new InfluxDB({ url: env.INFLUX_URL, token: env.INFLUX_TOKEN });
  return influx.getQueryApi(env.INFLUX_ORG);
}


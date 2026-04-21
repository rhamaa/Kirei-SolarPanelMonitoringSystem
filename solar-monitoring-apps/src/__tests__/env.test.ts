import { describe, expect, it, vi } from "vitest";

vi.mock("server-only", () => ({}));

function setBaseEnv(overrides: Partial<NodeJS.ProcessEnv> = {}) {
  process.env = {
    ...process.env,
    INFLUX_URL: "http://localhost:8086",
    INFLUX_ORG: "org",
    INFLUX_BUCKET: "bucket",
    INFLUX_TOKEN: "super-secret-token",
    DEFAULT_DEVICE_ID: "device_1",
    ...overrides,
  };
}

async function importFreshEnv() {
  vi.resetModules();
  return await import("../../lib/env");
}

describe("env", () => {
  it("returns typed values for valid env (with defaults)", async () => {
    setBaseEnv({ KPI_RANGE_DEFAULT: undefined, KPI_REFRESH_MS: undefined });

    const { env } = await importFreshEnv();

    expect(env.INFLUX_URL).toBe("http://localhost:8086");
    expect(env.KPI_RANGE_DEFAULT).toBe("24h");
    expect(env.KPI_REFRESH_MS).toBe(10_000);
  });

  it("throws an actionable error mentioning missing var names only", async () => {
    const tokenValue = "my-real-token-value";
    setBaseEnv({ INFLUX_TOKEN: tokenValue });

    delete process.env.INFLUX_TOKEN;

    await expect(importFreshEnv()).rejects.toThrow(/INFLUX_TOKEN/);
    await expect(importFreshEnv()).rejects.not.toThrow(new RegExp(tokenValue));
  });
});


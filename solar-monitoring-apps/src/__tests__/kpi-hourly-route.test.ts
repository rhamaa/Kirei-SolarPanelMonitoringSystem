import { describe, expect, it, vi } from "vitest";

vi.mock("server-only", () => ({}));

type CollectRows = (flux: string) => Promise<Array<Record<string, unknown>>>;
const collectRowsMock = vi.fn<CollectRows>();

vi.mock("@/lib/influx/client", () => ({
  getQueryApi: vi.fn(async () => ({ collectRows: collectRowsMock })),
}));

function setBaseEnv(overrides: Partial<NodeJS.ProcessEnv> = {}) {
  process.env = {
    ...process.env,
    INFLUX_URL: "http://localhost:8086",
    INFLUX_ORG: "org",
    INFLUX_BUCKET: "bucket",
    INFLUX_TOKEN: "super-secret-token",
    DEFAULT_DEVICE_ID: "device_1",
    KPI_RANGE_DEFAULT: "24h",
    ...overrides,
  };
}

async function importFreshRoute() {
  vi.resetModules();
  return await import("../../app/api/kpi/hourly/route");
}

describe("GET /api/kpi/hourly", () => {
  it("returns normalized hourly JSON with points array", async () => {
    setBaseEnv();
    collectRowsMock.mockReset();
    collectRowsMock.mockResolvedValueOnce([
      { _time: "2026-01-15T10:00:00.000Z", _value: 100 },
      { _time: "2026-01-15T11:00:00.000Z", _value: 120 },
    ]);

    const { GET } = await importFreshRoute();
    const res = await GET(
      new Request("http://localhost/api/kpi/hourly?device_id=device_1&field=mppt_charging_power&range=24h"),
    );
    const body = (await res.json()) as Record<string, unknown>;

    expect(res.status).toBe(200);
    expect(body.deviceId).toBe("device_1");
    expect(body.field).toBe("mppt_charging_power");
    expect(body.range).toBe("24h");
    expect(body.aggregateEvery).toBe("1h");
    expect(body.bucketDurationHours).toBe(1);
    expect(Array.isArray(body.points)).toBe(true);
    expect(JSON.stringify(body)).not.toMatch(/super-secret-token/);
  });

  it("returns 502 when Influx fails", async () => {
    setBaseEnv();
    collectRowsMock.mockReset();
    collectRowsMock.mockRejectedValueOnce(new Error("boom"));

    const { GET } = await importFreshRoute();
    const res = await GET(new Request("http://localhost/api/kpi/hourly?device_id=device_1"));
    const body = (await res.json()) as Record<string, unknown>;

    expect(res.status).toBe(502);
    expect(body.error).toBe("influx_unavailable");
  });
});

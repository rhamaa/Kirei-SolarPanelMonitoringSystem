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
    ...overrides,
  };
}

async function importFreshRoute() {
  vi.resetModules();
  return await import("../../app/api/kpi/latest/route");
}

describe("GET /api/kpi/latest", () => {
  it("rejects invalid device_id values", async () => {
    setBaseEnv();
    collectRowsMock.mockReset();
    collectRowsMock.mockResolvedValueOnce([]);

    const { GET } = await importFreshRoute();
    const res = await GET(new Request("http://localhost/api/kpi/latest?device_id=bad$&range=24h"));

    expect(res.status).toBe(400);
  });

  it("rejects forbidden query keys that could lead to SSRF/secret leakage", async () => {
    setBaseEnv();
    collectRowsMock.mockReset();
    collectRowsMock.mockResolvedValueOnce([]);

    const { GET } = await importFreshRoute();
    const res = await GET(new Request("http://localhost/api/kpi/latest?token=abc"));

    expect(res.status).toBe(400);
  });

  it("returns 500 JSON when env is missing (names only)", async () => {
    setBaseEnv();
    collectRowsMock.mockReset();
    delete process.env.INFLUX_URL;

    const { GET } = await importFreshRoute();
    const res = await GET(new Request("http://localhost/api/kpi/latest"));
    const body = (await res.json()) as Record<string, unknown>;

    expect(res.status).toBe(500);
    expect(body.error).toBe("env_invalid");
    expect(JSON.stringify(body)).toMatch(/INFLUX_URL/);
    expect(JSON.stringify(body)).not.toMatch(/super-secret-token/);
  });

  it("returns normalized JSON with no secret fields", async () => {
    setBaseEnv();
    collectRowsMock.mockReset();
    collectRowsMock.mockResolvedValueOnce([]);

    const { GET } = await importFreshRoute();
    const res = await GET(new Request("http://localhost/api/kpi/latest?device_id=device_1&range=24h"));
    const body = (await res.json()) as Record<string, unknown>;

    expect(res.status).toBe(200);
    const keys = Object.keys(body);
    expect(keys).toEqual(expect.arrayContaining(["deviceId", "range", "asOf", "values"]));
    expect(JSON.stringify(body)).not.toMatch(/token|org|bucket|url/i);
  });

  it("returns 502 with stable error code when Influx fails", async () => {
    setBaseEnv();
    collectRowsMock.mockReset();
    collectRowsMock.mockRejectedValueOnce(new Error("unauthorized"));

    const { GET } = await importFreshRoute();
    const res = await GET(new Request("http://localhost/api/kpi/latest?device_id=device_1&range=24h"));
    const body = (await res.json()) as Record<string, unknown>;

    expect(res.status).toBe(502);
    expect(body.error).toBe("influx_unavailable");
  });
});


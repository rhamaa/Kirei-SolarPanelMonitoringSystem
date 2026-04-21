import { describe, expect, it } from "vitest";

import { buildLatestKpiFlux } from "../../lib/influx/queries";

describe("buildLatestKpiFlux", () => {
  it("includes measurement, device filter, last(), and pivot()", () => {
    const flux = buildLatestKpiFlux({
      bucket: "bucket",
      deviceId: "device_1",
      range: "24h",
    });

    expect(flux).toContain('r._measurement == "pv_monitoring"');
    expect(flux).toContain('r.device_id == "device_1"');
    expect(flux).toContain("|> last()");
    expect(flux).toContain("|> pivot(");
  });

  it("maps range presets into range(start: -preset)", () => {
    const flux = buildLatestKpiFlux({
      bucket: "bucket",
      deviceId: "device_1",
      range: "7d",
    });

    expect(flux).toContain("|> range(start: -7d)");
  });
});


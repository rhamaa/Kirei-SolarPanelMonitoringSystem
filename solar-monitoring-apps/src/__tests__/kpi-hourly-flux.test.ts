import { describe, expect, it } from "vitest";

import { buildHourlyMeanFlux } from "../../lib/influx/queries";

describe("buildHourlyMeanFlux", () => {
  it("scopes measurement, device, field, and mean window matching dashboard range", () => {
    const flux24 = buildHourlyMeanFlux({
      bucket: "b",
      deviceId: "pv-01",
      field: "mppt_charging_power",
      range: "24h",
    });

    expect(flux24).toContain("|> range(start: -24h)");
    expect(flux24).toContain('r._measurement == "pv_monitoring"');
    expect(flux24).toContain('r.device_id == "pv-01"');
    expect(flux24).toContain('r._field == "mppt_charging_power"');
    expect(flux24).toContain("|> aggregateWindow(every: 1h, fn: mean, createEmpty: true)");
  });

  it("uses shorter buckets for 1h and longer for 7d", () => {
    const flux1h = buildHourlyMeanFlux({ bucket: "b", deviceId: "x", field: "mppt_pv_voltage", range: "1h" });
    expect(flux1h).toContain("|> range(start: -1h)");
    expect(flux1h).toContain("|> aggregateWindow(every: 2m, fn: mean, createEmpty: true)");

    const flux7d = buildHourlyMeanFlux({ bucket: "b", deviceId: "x", field: "mppt_pv_voltage", range: "7d" });
    expect(flux7d).toContain("|> range(start: -7d)");
    expect(flux7d).toContain("|> aggregateWindow(every: 6h, fn: mean, createEmpty: true)");
  });
});

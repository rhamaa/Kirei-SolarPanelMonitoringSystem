import { describe, expect, it } from "vitest";

import { buildHourlyMeanFlux } from "../../lib/influx/queries";

describe("buildHourlyMeanFlux", () => {
  it("scopes measurement mppt, device tag, influx field, and mean window matching dashboard range", () => {
    const flux24 = buildHourlyMeanFlux({
      bucket: "b",
      deviceId: "kirei_solar_panel",
      field: "mppt_charging_power",
      range: "24h",
    });

    expect(flux24).toContain("|> range(start: -24h)");
    expect(flux24).toContain('r._measurement == "mppt"');
    expect(flux24).toContain('r["device"] == "kirei_solar_panel"');
    expect(flux24).toContain('r._field == "charging_power"');
    expect(flux24).toContain("|> aggregateWindow(every: 1h, fn: mean, createEmpty: true)");
  });

  it("uses inverter measurement for AC power field id", () => {
    const flux = buildHourlyMeanFlux({
      bucket: "b",
      deviceId: "x",
      field: "inverter_ac_power",
      range: "6h",
    });
    expect(flux).toContain('r._measurement == "inverter"');
    expect(flux).toContain('r._field == "power"');
    expect(flux).toContain("|> aggregateWindow(every: 15m, fn: mean, createEmpty: true)");
  });

  it("uses shorter buckets for 1h and longer for 7d", () => {
    const flux1h = buildHourlyMeanFlux({ bucket: "b", deviceId: "x", field: "mppt_pv_voltage", range: "1h" });
    expect(flux1h).toContain("|> range(start: -1h)");
    expect(flux1h).toContain('r._field == "pv_voltage"');
    expect(flux1h).toContain("|> aggregateWindow(every: 2m, fn: mean, createEmpty: true)");

    const flux7d = buildHourlyMeanFlux({ bucket: "b", deviceId: "x", field: "mppt_pv_voltage", range: "7d" });
    expect(flux7d).toContain("|> range(start: -7d)");
    expect(flux7d).toContain("|> aggregateWindow(every: 6h, fn: mean, createEmpty: true)");
  });
});

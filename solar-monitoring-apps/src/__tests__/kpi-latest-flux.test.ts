import { describe, expect, it } from "vitest";

import { buildLatestInverterLastFieldsFlux, buildLatestMpptLastFieldsFlux } from "../../lib/influx/queries";

describe("buildLatestMpptLastFieldsFlux / buildLatestInverterLastFieldsFlux", () => {
  it("scopes measurement, device tag, group by field, and last()", () => {
    const mppt = buildLatestMpptLastFieldsFlux({
      bucket: "bucket",
      deviceId: "kirei_solar_panel",
      range: "24h",
    });

    expect(mppt).toContain('from(bucket: "bucket")');
    expect(mppt).toContain("|> range(start: -24h)");
    expect(mppt).toContain('r._measurement == "mppt"');
    expect(mppt).toContain('r["device"] == "kirei_solar_panel"');
    expect(mppt).toContain('|> group(columns: ["_field"])');
    expect(mppt).toContain("|> last()");

    const inv = buildLatestInverterLastFieldsFlux({
      bucket: "bucket",
      deviceId: "kirei_solar_panel",
      range: "24h",
    });

    expect(inv).toContain('r._measurement == "inverter"');
    expect(inv).toContain('r["device"] == "kirei_solar_panel"');
  });

  it("maps range presets into range(start: -preset)", () => {
    const flux = buildLatestMpptLastFieldsFlux({
      bucket: "bucket",
      deviceId: "device_1",
      range: "7d",
    });

    expect(flux).toContain("|> range(start: -7d)");
  });
});

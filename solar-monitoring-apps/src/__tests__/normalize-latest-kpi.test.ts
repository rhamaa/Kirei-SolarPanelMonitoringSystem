import { describe, expect, it } from "vitest";

import { normalizeLatestKpi } from "../../lib/influx/normalize";

describe("normalizeLatestKpi", () => {
  it("maps firmware Influx field names (mppt_*, inverter_ac_*) to dashboard keys", () => {
    const asOf = new Date("2026-01-15T12:00:00.000Z");
    const body = normalizeLatestKpi({
      deviceId: "pv-monitoring-01",
      range: "1h",
      asOf,
      rows: [
        {
          _time: "2026-01-15T11:59:00.000Z",
          device_id: "pv-monitoring-01",
          mppt_pv_voltage: 48.2,
          mppt_charging_current: 3.1,
          mppt_charging_power: 149.42,
          mppt_battery_voltage: 51.0,
          inverter_ac_power: 120.5,
        },
      ],
    });

    expect(body.values.pv_voltage).toEqual({
      value: 48.2,
      unit: "V",
      time: "2026-01-15T11:59:00.000Z",
    });
    expect(body.values.pv_current?.value).toBe(3.1);
    expect(body.values.pv_power?.value).toBe(149.42);
    expect(body.values.battery_voltage?.value).toBe(51);
    expect(body.values.inverter_power?.value).toBe(120.5);

    expect(body.snapshot.mppt_pv_voltage).toBe(48.2);
    expect(body.snapshot.mppt_charging_current).toBe(3.1);
    expect(body.snapshot.mppt_charging_power).toBe(149.42);
    expect(body.snapshot.mppt_battery_voltage).toBe(51);
    expect(body.snapshot.inverter_ac_power).toBe(120.5);
    expect(body.snapshot.inverter_ac_voltage).toBeNull();
  });

  it("still accepts canonical keys when present (no alias)", () => {
    const asOf = new Date("2026-01-15T12:00:00.000Z");
    const body = normalizeLatestKpi({
      deviceId: "d1",
      range: "24h",
      asOf,
      rows: [
        {
          _time: "2026-01-15T11:00:00.000Z",
          pv_voltage: 12,
          pv_current: 1,
          pv_power: 10,
          battery_voltage: 13,
          inverter_power: 5,
        },
      ],
    });

    expect(body.values.pv_voltage?.value).toBe(12);
    expect(body.values.inverter_power?.value).toBe(5);
  });

  it("parses numeric strings from Flux/JSON", () => {
    const asOf = new Date("2026-01-15T12:00:00.000Z");
    const body = normalizeLatestKpi({
      deviceId: "d1",
      range: "1h",
      asOf,
      rows: [
        {
          _time: "2026-01-15T11:00:00.000Z",
          mppt_pv_voltage: "48.2",
        },
      ],
    });

    expect(body.values.pv_voltage?.value).toBe(48.2);
  });

  it("returns null snapshot fields when row is missing", () => {
    const body = normalizeLatestKpi({
      deviceId: "d1",
      range: "1h",
      asOf: new Date("2026-01-15T12:00:00.000Z"),
      rows: [],
    });
    expect(body.snapshot.mppt_pv_voltage).toBeNull();
  });
});

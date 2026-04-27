import { describe, expect, it } from "vitest";

import { mergeLatestFieldRowsToCanonicalRow } from "../../lib/influx/influx-schema";

describe("mergeLatestFieldRowsToCanonicalRow", () => {
  it("maps mppt + inverter last-per-field rows into DeviceSnapshot-shaped keys", () => {
    const merged = mergeLatestFieldRowsToCanonicalRow(
      [
        { _field: "pv_voltage", _value: 99.1, _time: "2026-01-15T10:00:00.000Z" },
        { _field: "battery_soc", _value: 76, _time: "2026-01-15T10:05:00.000Z" },
        { _field: "charging_power", _value: 200, _time: "2026-01-15T10:05:00.000Z" },
      ],
      [
        { _field: "power", _value: 180, _time: "2026-01-15T10:04:00.000Z" },
        { _field: "voltage", _value: 230, _time: "2026-01-15T10:04:00.000Z" },
        { _field: "current", _value: 0.8, _time: "2026-01-15T10:04:00.000Z" },
        { _field: "frequency", _value: 50.01, _time: "2026-01-15T10:04:00.000Z" },
      ],
    );

    expect(merged.mppt_pv_voltage).toBe(99.1);
    expect(merged.mppt_battery_soc).toBe(76);
    expect(merged.mppt_charging_power).toBe(200);
    expect(merged.inverter_ac_power).toBe(180);
    expect(merged.inverter_ac_voltage).toBe(230);
    expect(merged.inverter_ac_current).toBe(0.8);
    expect(merged.inverter_ac_frequency).toBe(50.01);
    expect(merged.inverter_ac_apparent_power).toBeCloseTo(184, 5);
    expect(merged.inverter_valid).toBe(1);
    expect(merged._time).toBe("2026-01-15T10:05:00.000Z");
  });
});

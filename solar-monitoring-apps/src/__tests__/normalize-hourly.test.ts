import { describe, expect, it } from "vitest";

import { normalizeHourlyMeanSeries } from "../../lib/influx/normalize-hourly";

describe("normalizeHourlyMeanSeries", () => {
  it("sorts by time and maps _value to value", () => {
    const asOf = new Date("2026-01-15T12:00:00.000Z");
    const body = normalizeHourlyMeanSeries({
      deviceId: "d1",
      field: "mppt_charging_power",
      asOf,
      rows: [
        { _time: "2026-01-15T11:00:00.000Z", _value: 10 },
        { _time: "2026-01-15T09:00:00.000Z", _value: 5 },
      ],
    });

    expect(body.points.map((p) => p.time)).toEqual(["2026-01-15T09:00:00.000Z", "2026-01-15T11:00:00.000Z"]);
    expect(body.points.map((p) => p.value)).toEqual([5, 10]);
  });
});

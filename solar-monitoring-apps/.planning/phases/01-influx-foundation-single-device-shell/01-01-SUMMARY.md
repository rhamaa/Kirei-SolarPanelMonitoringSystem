---
phase: 01-influx-foundation-single-device-shell
plan: 01
subsystem: solar-monitoring-apps
tags: [influxdb, nextjs, api, env, vitest, security]
requires: []
provides:
  - server-only Influx env validation (`lib/env.ts`)
  - server-only Influx query layer (`lib/influx/*`)
  - latest KPI endpoint (`app/api/kpi/latest/route.ts`)
affects:
  - solar-monitoring-apps/package.json
  - solar-monitoring-apps/pnpm-lock.yaml
  - solar-monitoring-apps/vitest.config.ts
  - solar-monitoring-apps/lib/env.ts
  - solar-monitoring-apps/lib/influx/client.ts
  - solar-monitoring-apps/lib/influx/queries.ts
  - solar-monitoring-apps/lib/influx/normalize.ts
  - solar-monitoring-apps/app/api/kpi/latest/route.ts
  - solar-monitoring-apps/src/__tests__/env.test.ts
  - solar-monitoring-apps/src/__tests__/kpi-latest-flux.test.ts
  - solar-monitoring-apps/src/__tests__/kpi-latest-route.test.ts
tech_stack:
  - Next.js App Router route handler (Node runtime)
  - @influxdata/influxdb-client (QueryApi.collectRows)
  - @t3-oss/env-nextjs + zod (server-only env validation)
  - vitest (node test runner)
key_decisions:
  - Keep Influx secrets server-only via `import "server-only"` guarded modules.
  - Limit API query params to `device_id` + preset `range` and reject Influx connection params to mitigate SSRF/secret leakage.
completed_at: "2026-04-21"
---

# Phase 01 Plan 01: Influx Foundation + Latest KPI API Summary

Established a secure server-side InfluxDB foundation and a single API surface that returns normalized “latest KPI” values for a device, with automated tests guarding secret leakage and SSRF patterns.

## What Shipped

- **Env validation (server-only)**: `solar-monitoring-apps/lib/env.ts`
  - Validates `INFLUX_URL`, `INFLUX_ORG`, `INFLUX_BUCKET`, `INFLUX_TOKEN`, `DEFAULT_DEVICE_ID`
  - Defaults `KPI_RANGE_DEFAULT` to `24h` and `KPI_REFRESH_MS` to `10_000` (min \(1000\))
  - Missing/invalid env fails fast with error messages containing variable names only
- **Influx query layer (server-only)**: `solar-monitoring-apps/lib/influx/*`
  - `client.ts`: creates `InfluxDB` + `QueryApi`
  - `queries.ts`: `buildLatestKpiFlux()` uses `last()` + `pivot()` against measurement `pv_monitoring`
  - `normalize.ts`: converts rows into stable DTO shape for the API
- **API route**: `solar-monitoring-apps/app/api/kpi/latest/route.ts`
  - `GET /api/kpi/latest?device_id=...&range=24h`
  - Only accepts `device_id` (strict charset/length) + `range` preset (`1h|6h|24h|7d`)
  - Rejects Influx connection params in query string (e.g. `token`, `url`, `bucket`)
  - Error mapping:
    - **500** for invalid/missing env with actionable message (names only)
    - **200** with null KPI entries when query returns no rows
    - **502** with stable `{ error: "influx_unavailable" }` on Influx failures

## Verification

- `cd solar-monitoring-apps && pnpm test`

## Commits

- `7cf5443` feat(01-influx-foundation-single-device-shell-01): add validated server env + Vitest
- `efc798e` feat(01-influx-foundation-single-device-shell-01): add latest KPI Influx API

## Deviations from Plan

None — executed as planned.

## Known Stubs

None.

## Threat Flags

None — no new externally-exposed endpoints beyond the planned `GET /api/kpi/latest`, and response/tests enforce “no secret fields” behavior.

## Self-Check: PASSED


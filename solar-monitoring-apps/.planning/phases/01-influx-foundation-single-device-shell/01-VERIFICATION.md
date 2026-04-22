---
phase: 01-influx-foundation-single-device-shell
verified: 2026-04-22T10:05:00Z
status: human_needed
score: 2/4 must-haves verified
overrides_applied: 0
human_verification:
  - test: "E2E: /dashboard loads KPIs from real Influx"
    expected: "With valid .env.local + reachable Influx + existing measurement pv_monitoring/tag device_id, clicking “Refresh KPIs” shows non-null KPI values and timestamps; changing device_id/range changes results."
    why_human: "Requires live InfluxDB connectivity + real dataset; cannot be proven via static inspection."
  - test: "Security: confirm no Influx token leaks to browser"
    expected: "No INFLUX_TOKEN (or token value) appears in browser DevTools (Network payloads, view-source, JS bundles). No NEXT_PUBLIC_* secrets present."
    why_human: "Bundle/network inspection is runtime/browser-dependent; static checks can reduce risk but cannot fully prove absence in built artifacts."
---

# Phase 1: Influx Foundation + Single-Device Shell Verification Report

**Phase Goal:** Operator dapat membuka dashboard single-device yang sudah terhubung ke Influx (server-side), memilih `device_id` + time range dasar, dan melihat KPI “latest values” tanpa ada secret bocor ke client.
**Verified:** 2026-04-22T10:05:00Z
**Status:** human_needed
**Re-verification:** No — initial verification

## Goal Achievement

### Observable Truths (ROADMAP Success Criteria)

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | Dashboard bisa berjalan dengan konfigurasi `.env.local` dan menolak start / menampilkan error yang jelas saat env Influx belum lengkap. | ✓ VERIFIED | `/dashboard` server page catches env import and renders “Influx isn’t configured” callout listing var names only (`app/dashboard/page.tsx`). API returns `500 { error: "env_invalid", message }` when env import fails (`app/api/kpi/latest/route.ts`). |
| 2 | User bisa memilih `device_id` (default dari env) dan time range preset (minimal: 24h) dari UI. | ✓ VERIFIED | `DashboardClient` takes `defaultDeviceId/defaultRange` from server page and exposes Device ID input + Range preset select including `24h` (`components/dashboard/DashboardClient.tsx`). |
| 3 | KPI cards menampilkan “latest point” untuk `device_id` terpilih dari Influx, dan refresh menghasilkan nilai yang konsisten. | ? NEEDS HUMAN | Wiring exists UI → `GET /api/kpi/latest` → Influx Flux query (`collectRows`) → normalization, but cannot validate real Influx connectivity/data shape without runtime. |
| 4 | Tidak ada Influx token yang muncul di browser DevTools (view-source/bundles/network payloads) dan tidak ada `NEXT_PUBLIC_*` secret leakage. | ? NEEDS HUMAN | Static checks: env is server-only (`lib/env.ts` + `import "server-only"`), Influx client is server-only (`lib/influx/client.ts`), API response DTO contains no secret fields (`normalizeLatestKpi` + route tests). Runtime/bundle inspection still required to fully confirm. |

**Score:** 2/4 truths verified

### Required Artifacts (exist + substantive + wired)

| Artifact | Expected | Status | Details |
|---------|----------|--------|---------|
| `solar-monitoring-apps/lib/env.ts` | Validated server-only env (Influx URL/org/bucket/token + defaults) | ✓ VERIFIED | Uses `createEnv` with `client: {}` and `import "server-only"`. Throws actionable error listing missing var names only. |
| `solar-monitoring-apps/lib/influx/client.ts` | Server-only InfluxDB client + QueryApi | ✓ VERIFIED | `import "server-only"`; `getQueryApi()` builds `InfluxDB({ url, token })` from server env. |
| `solar-monitoring-apps/lib/influx/queries.ts` | Flux query builder for latest KPIs | ✓ VERIFIED | Builds Flux with measurement `pv_monitoring`, device tag filter, `last()` and `pivot()`, range preset mapping; escapes string literals. |
| `solar-monitoring-apps/lib/influx/normalize.ts` | Normalize Influx rows → stable DTO | ✓ VERIFIED | Produces `{ deviceId, range, asOf, values }` map with units and timestamps; null-safe when no rows. |
| `solar-monitoring-apps/app/api/kpi/latest/route.ts` | `GET /api/kpi/latest` route handler (Node runtime) | ✓ VERIFIED | `runtime="nodejs"`, validates `device_id` + preset `range`, forbids Influx connection params, calls `collectRows`, returns normalized JSON, 500/502 error mapping. |
| `solar-monitoring-apps/app/dashboard/page.tsx` | `/dashboard` server route with config-missing UX | ✓ VERIFIED | Imports env server-side in try/catch; passes only `DEFAULT_DEVICE_ID` + `KPI_RANGE_DEFAULT` to client; otherwise renders callout listing var names only. |
| `solar-monitoring-apps/components/dashboard/DashboardClient.tsx` | Client selectors + refresh + fetch to API | ✓ VERIFIED | `"use client"`; validates device_id; fetches `/api/kpi/latest`; renders loading skeletons, empty state, error with retry; no Influx connection inputs. |
| `solar-monitoring-apps/components/kpi/KpiCard.tsx` | KPI card UI (value/unit/time + skeleton) | ✓ VERIFIED | Skeleton + formatted value + “Last updated …”. |
| `solar-monitoring-apps/components/kpi/KpiGrid.tsx` | KPI grid layout | ✓ VERIFIED | 2-col desktop / 1-col small; renders `KpiCard`s. |

### Key Link Verification (wiring)

| From | To | Via | Status | Details |
|------|----|-----|--------|---------|
| `components/dashboard/DashboardClient.tsx` | `/api/kpi/latest` | `fetch` | ✓ WIRED | Uses `new URL("/api/kpi/latest", window.location.origin)` + `fetch(url.toString(), { cache: "no-store" })`. |
| `app/dashboard/page.tsx` | `lib/env.ts` | server-only import | ✓ WIRED | `await import("@/lib/env")` inside server component try/catch. |
| `app/api/kpi/latest/route.ts` | `lib/influx/*` | server import + query execution | ✓ WIRED | Imports `getQueryApi`, `buildLatestKpiFlux`, `normalizeLatestKpi`, and calls `queryApi.collectRows(flux)`. |

### Data-Flow Trace (Level 4)

| Artifact | Data Variable | Source | Produces Real Data | Status |
|---------|---------------|--------|--------------------|--------|
| `DashboardClient.tsx` | `data` (LatestKpiResponse) | `/api/kpi/latest` | ? | Connected (fetch + state set), but requires live Influx for non-null values. |
| `route.ts` | `rows` | Influx QueryApi (`collectRows`) | ? | Query execution is implemented; cannot prove result correctness without live Influx + dataset. |

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
|----------|---------|--------|--------|
| Unit tests for env/flux/route constraints | `pnpm -C solar-monitoring-apps test` | 3 files, 9 tests passing | ✓ PASS |

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|------------|-------------|-------------|--------|---------|
| INFLUX-01 | 01-01-PLAN | Server bisa query InfluxDB v2 (server-side only) | ✓ SATISFIED | `lib/influx/client.ts` uses `@influxdata/influxdb-client` with server-only env; API route uses it server-side. |
| INFLUX-02 | 01-01-PLAN | Bisa ambil “latest point” untuk `device_id` (measurement `pv_monitoring`) | ? NEEDS HUMAN | Flux builder targets `pv_monitoring` + `device_id` + `last()` + `pivot()`; requires live data to validate actual latest-point semantics. |
| UI-01 | 01-02-PLAN | KPI cards (latest values) | ? NEEDS HUMAN | UI renders KPI cards and states; needs live API data to confirm non-null values display end-to-end. |
| UI-03 | 01-02-PLAN | User bisa memilih device_id + time range preset | ✓ SATISFIED | Device ID input + range select presets in `DashboardClient.tsx`; defaults come from server env. |
| OPS-01 | 01-01-PLAN | Konfigurasi via `.env.local` + clear missing env error | ✓ SATISFIED | `lib/env.ts` validates required vars; `/dashboard` shows config callout; API returns `env_invalid` JSON. |
| OPS-02 | 01-01/02-PLAN | Tidak ada secret token terkirim ke browser bundle | ? NEEDS HUMAN | Static constraints enforced (server-only modules + tests assert response has no secret keys). Must confirm in browser bundles/network. |

### Anti-Patterns Found

No blocking stubs/placeholder implementations found in shipped code files for Phase 1. (Mentions of “placeholder” exist only in planning docs/roadmap text.)

### Human Verification Required

#### 1) E2E: /dashboard loads KPIs from real Influx

**Test:** Set valid `.env.local` (Influx URL/org/bucket/token + DEFAULT_DEVICE_ID), run app, open `/dashboard`, click “Refresh KPIs”, try different `device_id` and ranges.  
**Expected:** KPI cards show non-null values and meaningful timestamps; refresh is stable/consistent for same device/range; empty/error states behave as designed.  
**Why human:** Needs real Influx connectivity + real dataset.

#### 2) Security: confirm no Influx token leaks to browser

**Test:** In browser DevTools, check Network responses, view-source, and JS bundles for token string patterns; confirm no `NEXT_PUBLIC_*` secret usage.  
**Expected:** No token present anywhere client-side.  
**Why human:** Requires runtime inspection of built assets and network payloads.

---

_Verified: 2026-04-22T10:05:00Z_  
_Verifier: Claude (gsd-verifier)_


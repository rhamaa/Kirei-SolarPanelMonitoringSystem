---
phase: 01-influx-foundation-single-device-shell
plan: "02"
subsystem: frontend
tags: [nextjs, dashboard, ui, kpi]
requires: ["01-01"]
provides: ["/dashboard UI shell"]
affects:
  - solar-monitoring-apps/app/dashboard/page.tsx
  - solar-monitoring-apps/components/dashboard/DashboardClient.tsx
  - solar-monitoring-apps/components/kpi/KpiCard.tsx
  - solar-monitoring-apps/components/kpi/KpiGrid.tsx
  - solar-monitoring-apps/lib/kpi/types.ts
  - solar-monitoring-apps/app/globals.css
decisions:
  - "Keep env secrets server-only; `/dashboard` server page only passes non-secrets (DEFAULT_DEVICE_ID + range preset) into the client bundle."
  - "Avoid auto-fetch on mount due to repo ESLint rule forbidding setState in effects; KPI load is user-triggered via “Refresh KPIs”."
tech_stack:
  - Next.js App Router (RSC + client components)
  - Tailwind CSS v4 utilities
key_files:
  created:
    - solar-monitoring-apps/app/dashboard/page.tsx
    - solar-monitoring-apps/components/dashboard/DashboardClient.tsx
    - solar-monitoring-apps/components/kpi/KpiCard.tsx
    - solar-monitoring-apps/components/kpi/KpiGrid.tsx
    - solar-monitoring-apps/lib/kpi/types.ts
  modified:
    - solar-monitoring-apps/app/globals.css
metrics:
  completed_at: "2026-04-22"
---

# Phase 01 Plan 02: Dashboard shell + KPI cards wired to API — Summary

Built the Phase 1 operator UI at `/dashboard` with device + time-range controls and KPI cards consuming `GET /api/kpi/latest`, including dedicated states for env-misconfig, loading skeletons, empty data, and errors — without leaking any Influx secrets to the client.

## What Shipped

- **`/` → `/dashboard`** redirect via `app/page.tsx`.
- **Config-missing UX** on `/dashboard`:
  - Heading: “Influx isn’t configured”
  - Body copy per UI-SPEC
  - Required env var names listed (names only; no values).
- **Interactive dashboard client**:
  - Device ID manual input with local validation
  - Range preset selector (`1h`, `6h`, `24h`, `7d`) with Phase 2 helper text for custom ranges
  - Primary CTA: “Refresh KPIs”
  - Fetches `/api/kpi/latest?device_id=...&range=...` with `cache: "no-store"`
  - KPI grid: 2 columns desktop / 1 column small screens
  - States: skeleton loading, empty (all null values), error with “Retry”
- **Typography** updated so Geist Sans is used (no hardcoded Arial fallback in `globals.css`).

## Task Execution (Atomic Commits)

| Task | Name | Commit | Files |
| ---- | ---- | ------ | ----- |
| 1 | Add `/dashboard` route that renders config-missing callout or the interactive dashboard client | 88ee289 | `app/dashboard/page.tsx`, `components/dashboard/DashboardClient.tsx` |
| 2 | Implement dashboard client (selectors + refresh) + KPI cards wired to `GET /api/kpi/latest` | 8dcb42b | `components/dashboard/DashboardClient.tsx`, `components/kpi/*`, `lib/kpi/types.ts`, `app/globals.css`, `app/dashboard/page.tsx` |

## Verification

- `pnpm build` (after Task 1): **PASS**
- `pnpm lint` (after Task 2): **PASS**

## Deviations from Plan

None — plan executed as written.

## Threat Flags

None observed (no new secrets crossing the server→client boundary; no new endpoints beyond the planned `/api/kpi/latest` consumer).

## Self-Check

PASSED

- FOUND: `.planning/phases/01-influx-foundation-single-device-shell/01-02-SUMMARY.md`
- FOUND commits: `88ee289`, `8dcb42b`


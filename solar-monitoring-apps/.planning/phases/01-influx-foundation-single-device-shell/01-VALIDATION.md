# Phase 1: Influx Foundation + Single-Device Shell - Validation

**Created:** 2026-04-21  
**Nyquist:** enabled  
**Purpose:** Make verification explicit + automated where possible.

## Validation Architecture

### Commands (project-level)

- **Install deps**: `pnpm install`
- **Lint**: `pnpm lint`
- **Unit tests**: `pnpm test`
- **Build**: `pnpm build`

### Required env for integration-style checks

These MUST be set server-side (never `NEXT_PUBLIC_*`):

- `INFLUX_URL`
- `INFLUX_ORG`
- `INFLUX_BUCKET`
- `INFLUX_TOKEN`
- `DEFAULT_DEVICE_ID`

## Validation Matrix (per plan)

### Plan `01-01-PLAN.md` — Server-only Influx foundation + API

**Goal linkage:** INFLUX-01, INFLUX-02, OPS-01, OPS-02

- **V-01 (env validation)**:
  - **Automated**: `pnpm test -t env`
  - **Pass when**: tests prove missing env throws actionable error and server-only boundary is enforced.

- **V-02 (Flux builder / query normalization)**:
  - **Automated**: `pnpm test -t latestKpiFlux`
  - **Pass when**: query builder includes measurement `pv_monitoring`, device filter, `last()` and `pivot()` (or equivalent) and normalizer produces stable DTO.

- **V-03 (API route / secrets)**:
  - **Automated**: `pnpm test -t noSecret`
  - **Pass when**: response JSON contains no token and client bundles/network payload never includes `INFLUX_TOKEN` (tested via string scan of build output or explicit unit assertion against returned payload).

- **V-04 (baseline CI-like checks)**:
  - **Automated**: `pnpm lint` and `pnpm build`
  - **Pass when**: both commands exit 0.

### Plan `01-02-PLAN.md` — Dashboard shell + selectors + KPI cards

**Goal linkage:** UI-01, UI-03

- **V-05 (UI wiring smoke)**:
  - **Automated**: `pnpm build`
  - **Pass when**: `/dashboard` page compiles and the fetch to `/api/kpi/latest` is server-safe (no client-side Influx import).

- **V-06 (optional component tests)**:
  - **Automated**: `pnpm test -t KpiCard` (only if implemented)
  - **Pass when**: KPI cards render expected labels/values from DTO and handle loading/error states.

## Manual Checks (allowed, but should be minimal)

- Open `/dashboard`:
  - With valid env: KPI cards show values and “Last updated …”.
  - With missing env: UI shows “Influx isn’t configured” callout and lists required env var names (without showing values).

---

*Validation created: 2026-04-21*


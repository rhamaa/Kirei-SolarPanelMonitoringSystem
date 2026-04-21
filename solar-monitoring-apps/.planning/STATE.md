---
gsd_state_version: 1.0
milestone: v1.0
milestone_name: milestone
status: unknown
last_updated: "2026-04-21T09:24:25.137Z"
progress:
  total_phases: 3
  completed_phases: 0
  total_plans: 2
  completed_plans: 0
  percent: 0
---

# STATE: Solar Monitoring Realtime Panel (Next.js)

## Project Reference

**Core value**: Operator bisa melihat nilai terbaru + tren PV/inverter secara realtime dan cepat, tanpa buka tool Influx manual.  
**Scope**: `solar-monitoring-apps/` (Next.js App Router)  
**Constraints**: pnpm, Influx via server-side `@influxdata/influxdb-client`, realtime via SSE, token never shipped to client.

## Current Position

Phase: 1 (Influx Foundation + Single-Device Shell) — EXECUTING
Plan: 1 of 2
**Current phase**: Phase 1 — Influx Foundation + Single-Device Shell  
**Status**: Planned (2 plans) — ready to execute  
**Progress**: \[----------\] 0%

## Blockers / External Dependencies

- **Influx credentials & reachability**: `INFLUX_URL`, `INFLUX_ORG`, `INFLUX_BUCKET`, `INFLUX_TOKEN` valid and reachable from the Next.js server runtime.
- **Data contract**: measurement `pv_monitoring`, tag `device_id`, and required fields available (or mapping confirmed).

## Decisions (locked for v1)

- **Realtime transport**: SSE (server → browser).
- **Security**: Influx token server-side only; never exposed to client bundles.
- **Scope**: single-device dashboard, default time range 24h, target realtime 1s.
- **No auth in v1**: LAN/internal usage first.

## Open Questions (answer during Phase 1/2 planning)

- What are the exact “field names” to chart for PV voltage, charging power, battery voltage, inverter power (confirm from Influx schema)?
- What is the acceptable staleness window before UI marks data as “stale” for this deployment?
- Should Phase 2 queries default to aggregation/downsampling to protect UI performance for dense series?

## Session Continuity

**Last activity**: UI-SPEC approved — `.planning/phases/01-influx-foundation-single-device-shell/01-UI-SPEC.md`  
**Next action**: Execute Phase 1 (`/gsd-execute-phase 1`) — run both plans (Wave 1 then Wave 2).  
**Guardrails**: avoid accepting any host/org/bucket/token parameters from client input (SSRF + secret leakage risk).

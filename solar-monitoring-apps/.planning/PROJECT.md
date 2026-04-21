# Solar Monitoring Realtime Panel (Next.js)

## What This Is

Web dashboard (Next.js) untuk memonitor data solar panel **realtime** dari InfluxDB (dan nantinya MQTT), fokus v1: **1 device** dengan tampilan panel + grafik historical (default 24 jam) dan stream update realtime ke browser.

## Core Value

Operator bisa melihat **nilai terbaru + tren** PV/inverter secara realtime dan cepat, tanpa buka tool Influx/MQTT manual.

## Requirements

### Validated

(None yet — ship to validate)

### Active

- [ ] Dashboard single-device: KPI cards + charts + time range
- [ ] Realtime update ke browser via **SSE**
- [ ] Data source: InfluxDB (wajib), MQTT (v1: optional/secondary)
- [ ] Config via environment variables (token tidak pernah dikirim ke client)
- [ ] Layout rapi, mobile-friendly, dan performa stabil di LAN

### Out of Scope

- Login/auth (v1) — akses LAN/internal dulu
- Multi-device fleet view (v1) — fokus 1 device dulu
- Alerting/notifications (v1) — nanti setelah panel stabil

## Context

- Workspace ini berisi firmware IoT + tools lain; project web ini **hanya** di `solar-monitoring-apps/`.
- Next.js app sudah ada (App Router), tetapi belum ada API routes / data fetching / realtime transport.
- Influx source (dari firmware): measurement `pv_monitoring`, tag `device_id`, fields MPPT & inverter.

## Constraints

- **Package manager**: `pnpm`
- **Influx client**: pakai `@influxdata/influxdb-client` (server-side only)
- **Realtime transport**: SSE (server → browser)
- **Security**: token Influx hanya di server (Next route handler), tidak pernah di bundle client
- **Default UX**: single device, default time range 24h, realtime stream target 1s (walau data masuk bisa 5s)

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| SSE untuk realtime | Sederhana, stabil, cocok server→client | — Pending |
| Single-device v1 | Fokus deliver core value cepat | — Pending |
| Tanpa auth v1 | Internal/LAN; iterasi cepat | — Pending |
| Query Influx via server route | Hindari expose token & CORS | — Pending |

## Evolution

This document evolves at phase transitions and milestone boundaries.

**After each phase transition** (via `/gsd-transition`):
1. Requirements invalidated? → Move to Out of Scope with reason
2. Requirements validated? → Move to Validated with phase reference
3. New requirements emerged? → Add to Active
4. Decisions to log? → Add to Key Decisions
5. "What This Is" still accurate? → Update if drifted

**After each milestone** (via `/gsd-complete-milestone`):
1. Full review of all sections
2. Core Value check — still the right priority?
3. Audit Out of Scope — reasons still valid?
4. Update Context with current state

---
*Last updated: 2026-04-21 after initialization*


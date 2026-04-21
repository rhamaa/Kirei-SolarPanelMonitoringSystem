# Phase 1: Influx Foundation + Single-Device Shell - Context

**Gathered:** 2026-04-21  
**Status:** Ready for planning  
**Source:** User instruction during `/gsd-plan-phase 1` (Influx-only)

<domain>
## Phase Boundary

Phase 1 fokus membuat fondasi koneksi **InfluxDB v2** yang aman (server-side) + dashboard shell single-device untuk menampilkan “latest KPI values”.

Tidak ada integrasi MQTT di project ini.
</domain>

<decisions>
## Implementation Decisions (locked)

### Data source & security
- InfluxDB adalah satu-satunya sumber data (Influx-only). **Tidak ada MQTT** (hapus koneksi MQTT dan jangan tambah dependency MQTT).
- Token Influx **server-side only** (Route Handler / Server Components). Tidak ada token di client bundle dan tidak ada `NEXT_PUBLIC_*` untuk secret.
- Env var yang dipakai (nama final ditentukan di Phase 1 plan, tapi wajib ada): URL, org, bucket, token, default device_id.

### API surface (internal)
- Client tidak boleh query Influx langsung. Semua query melalui endpoint/server function yang kita kontrol.
- Untuk Phase 1, minimal endpoint untuk “latest values” per `device_id`.

### UX v1
- Single-device dashboard.
- Default time range: 24h (walau Phase 1 mungkin hanya pakai preset + placeholder chart).

### Scope exclusions (Phase 1)
- Tidak membangun chart full historical (itu Phase 2).
- Tidak membangun SSE realtime stream (itu Phase 2).
</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Project
- `.planning/PROJECT.md` — scope & constraints (Influx-only)
- `.planning/REQUIREMENTS.md` — REQ IDs
- `.planning/ROADMAP.md` — Phase 1 goal + success criteria
- `CLAUDE.md` — project instructions

### Codebase map
- `.planning/codebase/ARCHITECTURE.md` — App Router baseline
- `.planning/codebase/STACK.md` — Next/React/TS/Tailwind versions
</canonical_refs>

<specifics>
## Specific Ideas

- Implementasi “latest KPI values” sebaiknya mengembalikan JSON yang sudah normalized (angka + timestamp + device_id).
- Validasi env Influx harus jelas: kalau env belum lengkap, tampilkan error yang actionable (di UI dan/atau server logs).
</specifics>

<deferred>
## Deferred Ideas

- MQTT status panel (removed from scope).
</deferred>

---

*Phase: 01-influx-foundation-single-device-shell*  
*Context gathered: 2026-04-21*


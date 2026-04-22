# Roadmap: Solar Monitoring Realtime Panel (Next.js)

**Granularity**: standard  
**Scope**: `solar-monitoring-apps/` only  
**v1 requirement coverage**: 13/13 mapped ✓

## Phases

- [ ] **Phase 1: Influx Foundation + Single-Device Shell** - Server-side Influx access + basic dashboard shell (device + range selection, KPI cards placeholder) + secure env setup
- [ ] **Phase 2: Charts + Realtime SSE (Stable UX)** - Historical time-series charts + SSE stream with reconnect/backoff + stale/error handling + load-safe polling strategy
- [ ] **Phase 3: Runbook + Ops Hardening** - Production/dev run docs + env checklist + final ops hardening (Influx-only)

## Phase Details

### Phase 1: Influx Foundation + Single-Device Shell
**Goal**: Operator dapat membuka dashboard single-device yang sudah terhubung ke Influx (server-side), memilih `device_id` + time range dasar, dan melihat KPI “latest values” tanpa ada secret bocor ke client.
**Depends on**: Nothing (first phase)
**Requirements**: INFLUX-01, INFLUX-02, UI-01, UI-03, OPS-01, OPS-02
**Success Criteria** (what must be TRUE):
  1. Dashboard bisa berjalan dengan konfigurasi `.env.local` dan menolak start / menampilkan error yang jelas saat env Influx belum lengkap.
  2. User bisa memilih `device_id` (default dari env) dan time range preset (minimal: 24h) dari UI.
  3. KPI cards menampilkan “latest point” untuk `device_id` terpilih dari Influx, dan refresh menghasilkan nilai yang konsisten.
  4. Tidak ada Influx token yang muncul di browser DevTools (view-source/bundles/network payloads) dan tidak ada `NEXT_PUBLIC_*` secret leakage.
**Plans**: 2 plans
Plans:
- [x] 01-01-PLAN.md — Secure env + server-only Influx query layer + `GET /api/kpi/latest` (with tests)
- [x] 01-02-PLAN.md — `/dashboard` shell + selectors + KPI cards wired to `/api/kpi/latest`
**UI hint**: yes

**External dependencies / blockers**:
- InfluxDB v2 reachable dari server (URL), plus `org`, `bucket`, `token` valid di `.env.local`.
- Dataset tersedia: measurement `pv_monitoring` dengan tag `device_id` dan fields yang dibutuhkan (atau mapping jelas).

### Phase 2: Charts + Realtime SSE (Stable UX)
**Goal**: Operator dapat melihat grafik historical (default 24h + custom range) dan menerima update realtime via SSE yang robust (auto-reconnect) tanpa membebani Influx berlebihan.
**Depends on**: Phase 1
**Requirements**: INFLUX-03, SSE-01, SSE-02, SSE-03, UI-02, UI-04
**Success Criteria** (what must be TRUE):
  1. User bisa melihat chart time-series untuk range yang dipilih (default 24h) untuk field utama, dan mengganti range mengubah data chart dengan benar.
  2. Browser menerima stream SSE untuk `device_id` terpilih, dan nilai KPI/indikator “last updated” berubah sesuai update terbaru.
  3. Saat koneksi SSE terputus, client auto-reconnect dengan backoff dan pulih tanpa refresh page.
  4. UI menampilkan state loading/error dan indikator “data stale” jika data tidak update dalam window yang wajar.
  5. Interval update dapat dikonfigurasi (target 1s) dan strategi pengambilan data tidak memicu beban Influx berlebihan untuk penggunaan LAN normal.
**Plans**: TBD
**UI hint**: yes

**External dependencies / blockers**:
- Influx query performance memadai untuk time range yang dipakai; jika tidak, perlu aturan downsample/aggregate (konfigurasi query) agar payload dan render tetap ringan.

### Phase 3: Runbook + Ops Hardening
**Goal**: Repo punya panduan run dev/prod + env yang jelas untuk operasi harian, dan dashboard siap dipakai Influx-only secara stabil.
**Depends on**: Phase 2
**Requirements**: OPS-03
**Success Criteria** (what must be TRUE):
  1. Dokumentasi menjalankan app (dev/prod) menjelaskan variabel environment yang dibutuhkan, contoh `.env.local`, dan cara verifikasi koneksi Influx.
  2. Dashboard tetap berfungsi penuh Influx-only tanpa error yang mengganggu saat env belum lengkap (pesan error jelas).
  3. Ada checklist troubleshooting umum (Influx unreachable, bucket/org salah, token invalid, empty dataset).
**Plans**: TBD
**UI hint**: yes

**External dependencies / blockers**:
- Tidak ada (Influx-only).

## Progress

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 1. Influx Foundation + Single-Device Shell | 1/2 | In Progress|  |
| 2. Charts + Realtime SSE (Stable UX) | 0/0 | Not started | - |
| 3. MQTT Optional + Runbook | 0/0 | Not started | - |

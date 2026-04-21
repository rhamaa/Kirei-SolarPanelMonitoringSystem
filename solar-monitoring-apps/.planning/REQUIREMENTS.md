# Requirements: Solar Monitoring Realtime Panel (Next.js)

**Defined:** 2026-04-21  
**Core Value:** Operator bisa melihat nilai terbaru + tren PV/inverter secara realtime dan cepat

## v1 Requirements

### Data Source (Influx)

- [ ] **INFLUX-01**: Server bisa query InfluxDB v2 menggunakan `@influxdata/influxdb-client` (token via env, server-side only)
- [ ] **INFLUX-02**: Bisa ambil “latest point” untuk `device_id` tertentu (measurement `pv_monitoring`)
- [ ] **INFLUX-03**: Bisa query time series untuk range (default 24h) untuk field utama (pv voltage, charging power, battery voltage, inverter power)

### Realtime (SSE)

- [ ] **SSE-01**: Ada endpoint SSE server-side yang mengirim update terbaru untuk 1 device
- [ ] **SSE-02**: Client auto-reconnect SSE saat putus (dengan backoff) tanpa refresh page
- [ ] **SSE-03**: Update interval SSE dapat dikonfigurasi (target 1s), dan tidak membebani Influx secara berlebihan

### Dashboard UI (Single Device)

- [ ] **UI-01**: Halaman utama menampilkan KPI cards (latest values) untuk MPPT & inverter
- [ ] **UI-02**: Halaman utama menampilkan chart time-series untuk range yang dipilih (default 24h)
- [ ] **UI-03**: User bisa memilih device (by `device_id`) dan time range (preset + custom)
- [ ] **UI-04**: UI menampilkan state loading/error dan status “data stale” jika data tidak update

### MQTT (Optional / Secondary)

- [ ] **MQTT-01**: (Opsional v1) Bisa tampilkan status/info terakhir dari MQTT topic `pv-monitoring/info` (read-only)

### Configuration & Ops

- [ ] **OPS-01**: Konfigurasi via `.env.local` (Influx URL/org/bucket/token, default device_id, refresh interval)
- [ ] **OPS-02**: Tidak ada secret (token) yang terkirim ke browser bundle
- [ ] **OPS-03**: Dokumentasi cara run dev/prod (pnpm) + environment variables

## v2 Requirements

### Multi Device

- **FLEET-01**: Overview multi-device (cards) + drilldown
- **FLEET-02**: Online/offline per device + last seen

### Alerts

- **ALERT-01**: Rule-based alerting (power drop, voltage abnormal) + notifikasi

## Out of Scope

| Feature | Reason |
|---------|--------|
| Login/auth v1 | Fokus internal/LAN dulu, percepat iterasi |
| Write-back control ke device (command) | Risky; butuh safety gate & audit |
| Mobile native app | Web-first |

## Traceability

| Requirement | Phase | Status |
|-------------|-------|--------|
| INFLUX-01 | Phase 1 | Pending |
| INFLUX-02 | Phase 1 | Pending |
| INFLUX-03 | Phase 2 | Pending |
| SSE-01 | Phase 2 | Pending |
| SSE-02 | Phase 2 | Pending |
| SSE-03 | Phase 2 | Pending |
| UI-01 | Phase 1 | Pending |
| UI-02 | Phase 2 | Pending |
| UI-03 | Phase 1 | Pending |
| UI-04 | Phase 2 | Pending |
| MQTT-01 | Phase 3 | Pending |
| OPS-01 | Phase 1 | Pending |
| OPS-02 | Phase 1 | Pending |
| OPS-03 | Phase 3 | Pending |

**Coverage:**
- v1 requirements: 14 total
- Mapped to phases: 14
- Unmapped: 0 ✓

---
*Requirements defined: 2026-04-21*  
*Last updated: 2026-04-21 after initial definition*


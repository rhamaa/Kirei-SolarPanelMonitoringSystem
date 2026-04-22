---
status: partial
phase: 01-influx-foundation-single-device-shell
source: [01-VERIFICATION.md]
started: 2026-04-22T00:00:00Z
updated: 2026-04-22T00:00:00Z
---

## Current Test

awaiting human testing

## Tests

### 1. E2E: `/dashboard` loads KPIs from real Influx
expected: App jalan dengan `.env.local` valid; klik “Refresh KPIs”; coba ganti `device_id` dan `range`; KPI cards menampilkan nilai non-null + timestamp masuk akal; refresh konsisten.
result: pending

### 2. Security: confirm no Influx token leaks to browser
expected: Tidak ada token Influx muncul di client (DevTools Network, view-source, JS bundles). Tidak ada secret via `NEXT_PUBLIC_*`.
result: pending

## Summary

total: 2
passed: 0
issues: 0
pending: 2
skipped: 0
blocked: 0

## Gaps


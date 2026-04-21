# Codebase Concerns (solar-monitoring-apps)

**Analysis Date:** 2026-04-21  
**Context:** current app is a minimal Next.js App Router template; concerns below are the highest-risk pitfalls when evolving it into a realtime dashboard backed by InfluxDB.

## Security Considerations (Dashboards + Influx + Realtime)

**Secret leakage to client bundles:**
- Risk: putting Influx tokens/URLs into client components (or exposing `NEXT_PUBLIC_*` incorrectly) leaks credentials.
- Current state: no env usage detected; `.env*` ignored in `solar-monitoring-apps/.gitignore`.
- Files to watch when features land: `app/**/*.tsx` (client components), `app/api/**/route.ts` (server route handlers), `next.config.ts`.

**Server-side request forgery (SSRF) via query endpoints:**
- Risk: if the UI can pass arbitrary Influx host/bucket/org/query parameters to an API route, attackers can pivot requests or exfiltrate data.
- Current state: no API routes detected (no `app/api/**/route.ts`).
- Mitigation direction (when it exists): fixed allowlists for org/bucket/measurement; never accept host/token from user input.

**Multi-tenant / access control gaps:**
- Risk: dashboards often add “deviceId / siteId” filters; without auth + row-level checks, users can query each other’s data.
- Current state: no auth/middleware detected (no `middleware.ts`, no auth deps).

## Performance Bottlenecks (Influx queries + UI rendering)

**Unbounded query ranges / cardinality:**
- Risk: naive “last 30 days raw points” queries can explode payload size and slow both server and browser.
- Current state: no query layer yet.
- Hotspots when added: `app/api/**/route.ts` (query endpoints), any chart components.

**Over-polling instead of push:**
- Risk: frequent polling creates load spikes and increases cost; can also cause UI jank.
- Current state: no data fetching detected (no `fetch` usage).
- Realtime note: choose a transport (SSE/WebSocket) deliberately; none is present today.

**Client-side rendering of huge time-series arrays:**
- Risk: chart libs can choke on tens/hundreds of thousands of points; React re-renders become expensive.
- Current state: no charting libs detected.

## Realtime Pitfalls (Next.js App Router)

**Connection lifecycle management:**
- Risk: WebSocket/SSE connections need careful lifecycle handling with App Router navigation and React 19 concurrency.
- Current state: no `"use client"` components detected; no realtime deps detected.

**Edge/runtime mismatches:**
- Risk: some realtime libraries or Influx clients expect Node APIs; deploying route handlers to Edge can break.
- Current state: no runtime config set (`solar-monitoring-apps/next.config.ts` is empty).

## Dashboard UX Fragility

**Missing error/loading shells:**
- Risk: realtime + data fetching requires consistent skeleton/error states.
- Current state: no `app/error.tsx`, `app/loading.tsx`, `app/not-found.tsx`.

## Dependencies / Capabilities Missing Today (explicit blockers)

- **InfluxDB client**: not present in `solar-monitoring-apps/package.json` (no `@influxdata/influxdb-client*`).
- **Realtime transport**: not present (no WebSocket/SSE helpers or libs like `socket.io`, `ws`, `pusher`, `ably`, etc).
- **Data fetching/caching layer**: not present (no SWR/TanStack Query; no `fetch` usage detected).
- **Testing**: not present (no unit/e2e test frameworks detected).

---

*Concerns audit: 2026-04-21*


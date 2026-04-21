# Quality Notes (solar-monitoring-apps)

**Analysis Date:** 2026-04-21  
**Scope:** current repo state only (no planned features)

## What’s Good

- **Modern baseline**: Next.js App Router + TypeScript strict (`solar-monitoring-apps/tsconfig.json`).
- **Linting present**: ESLint 9 + Next core-web-vitals preset (`solar-monitoring-apps/eslint.config.mjs`).
- **Styling baseline**: Tailwind v4 configured via PostCSS (`solar-monitoring-apps/postcss.config.mjs`, `solar-monitoring-apps/app/globals.css`).

## Code Quality Risks (current)

- **No test harness**: no unit/integration/e2e framework detected (no configs or deps in `solar-monitoring-apps/package.json`).
- **No app structure yet**: no `src/`, `components/`, or `lib/` directories detected; risk of ad-hoc growth as features are added.
- **No error/loading UX**: no `app/**/error.tsx`, `app/**/loading.tsx`, or `app/**/not-found.tsx` detected (limits resilience and perceived performance).
- **No env usage conventions**: `.env*` ignored (`solar-monitoring-apps/.gitignore`) but no `process.env.*` usage detected; easy to accidentally leak secrets to client components once they appear.

## Operational / DX Risks

- **Node version ambiguity**: no `engines` in `solar-monitoring-apps/package.json` and no Node version file; risk of “works on my machine” issues.
- **Config minimal**: `solar-monitoring-apps/next.config.ts` exists but is empty; no explicit hardening/perf settings captured yet.

## Quick Wins (low effort, high leverage)

- **Pin runtime**: add an `engines.node` field in `solar-monitoring-apps/package.json` (or a Node version file) to stabilize installs/builds.
- **Add basic route UX shells**: introduce `app/error.tsx`, `app/loading.tsx`, `app/not-found.tsx` to standardize UX once real pages/data exist.
- **Establish folders early**: create and use `components/`, `lib/`, and `app/(routes)/` conventions before feature work starts.
- **Add test runner before data logic**: pick a test runner (not currently present) before adding Influx queries + realtime transforms.

## Unknowns / Not Detected (explicit)

- **Auth/session strategy**: not detected (no middleware, no auth libs).
- **Data access patterns**: not detected (no `fetch`, no API routes, no server actions).
- **CI/CD**: not detected (no pipeline config found in this app directory).

---

*Quality map: 2026-04-21*


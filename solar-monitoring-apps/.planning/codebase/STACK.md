# Technology Stack (solar-monitoring-apps)

**Analysis Date:** 2026-04-21  
**Source of truth:** `solar-monitoring-apps/package.json` + config files in repo root

## Languages

**Primary:**
- TypeScript (configured in `solar-monitoring-apps/tsconfig.json`) - used by `app/layout.tsx`, `app/page.tsx`

**Secondary:**
- CSS (global styling in `solar-monitoring-apps/app/globals.css`)

## Runtime

**Environment:**
- Node.js: Version not pinned/detected (no `.nvmrc`, `.node-version`, or `engines` field found in `package.json`)

**Package Manager:**
- pnpm: lockfile present (`solar-monitoring-apps/pnpm-lock.yaml`)
- Workspace: `solar-monitoring-apps/pnpm-workspace.yaml` exists (minimal config)

## Frameworks

**Core:**
- Next.js `16.2.4` (`solar-monitoring-apps/package.json`)
  - App Router present (`solar-monitoring-apps/app/`)
- React `19.2.4` (`solar-monitoring-apps/package.json`)
- React DOM `19.2.4` (`solar-monitoring-apps/package.json`)

**Styling:**
- Tailwind CSS `^4` (`solar-monitoring-apps/package.json`)
  - Enabled via PostCSS plugin `@tailwindcss/postcss` (`solar-monitoring-apps/postcss.config.mjs`)
  - Tailwind imported via `@import "tailwindcss";` (`solar-monitoring-apps/app/globals.css`)
  - `tailwind.config.*`: Not detected (Tailwind v4 can run without it)

**Linting:**
- ESLint `^9` (`solar-monitoring-apps/package.json`)
- `eslint-config-next` `16.2.4` (`solar-monitoring-apps/package.json`)
- Config: `solar-monitoring-apps/eslint.config.mjs` (uses `eslint-config-next/core-web-vitals` + `eslint-config-next/typescript`)

**Testing:**
- Not detected (no Jest/Vitest/Playwright/Cypress deps or configs found)

## Key Dependencies

**Critical (current):**
- `next` `16.2.4` - routing, SSR/RSC runtime
- `react` `19.2.4` / `react-dom` `19.2.4` - UI runtime

**Infrastructure / Data:**
- Not detected (no API clients, DB clients, cache clients)

## Configuration

**TypeScript:**
- Strict mode enabled (`"strict": true`) in `solar-monitoring-apps/tsconfig.json`
- Path alias: `@/*` → `./*` (`solar-monitoring-apps/tsconfig.json`)

**Next.js config:**
- Present but empty: `solar-monitoring-apps/next.config.ts`

**Environment variables:**
- `.env*` are ignored (`solar-monitoring-apps/.gitignore`)
- No usage detected in code (no `process.env.*` references found)

## Platform Requirements

**Development:**
- Node.js + pnpm (implied by `pnpm-lock.yaml` + scripts in `package.json`)
- `node_modules/` currently exists in repo working tree (directory present)

**Production:**
- Not specified (no deployment config detected; Next defaults apply)

## Gaps vs. target: “realtime Influx dashboard”

**InfluxDB client:** Not detected (missing `@influxdata/influxdb-client` or `@influxdata/influxdb-client-browser`).  
**Realtime transport:** Not detected (missing dependencies/usage for WebSockets/SSE/Socket.IO/Pusher/Ably/etc).  
**Charting / dashboard UI:** Not detected (no chart libs like Recharts/ECharts/Chart.js/Visx).  
**State/data fetching layer:** Not detected (no SWR/TanStack Query; no `fetch` usage found).

---

*Stack analysis: 2026-04-21*


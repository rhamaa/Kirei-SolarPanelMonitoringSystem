<!-- GSD:project-start source:PROJECT.md -->
## Project

**Solar Monitoring Realtime Panel (Next.js)**

Web dashboard (Next.js) untuk memonitor data solar panel **realtime** dari InfluxDB (dan nantinya MQTT), fokus v1: **1 device** dengan tampilan panel + grafik historical (default 24 jam) dan stream update realtime ke browser.

**Core Value:** Operator bisa melihat **nilai terbaru + tren** PV/inverter secara realtime dan cepat, tanpa buka tool Influx/MQTT manual.

### Constraints

- **Package manager**: `pnpm`
- **Influx client**: pakai `@influxdata/influxdb-client` (server-side only)
- **Realtime transport**: SSE (server → browser)
- **Security**: token Influx hanya di server (Next route handler), tidak pernah di bundle client
- **Default UX**: single device, default time range 24h, realtime stream target 1s (walau data masuk bisa 5s)
<!-- GSD:project-end -->

<!-- GSD:stack-start source:codebase/STACK.md -->
## Technology Stack

## Languages
- TypeScript (configured in `solar-monitoring-apps/tsconfig.json`) - used by `app/layout.tsx`, `app/page.tsx`
- CSS (global styling in `solar-monitoring-apps/app/globals.css`)
## Runtime
- Node.js: Version not pinned/detected (no `.nvmrc`, `.node-version`, or `engines` field found in `package.json`)
- pnpm: lockfile present (`solar-monitoring-apps/pnpm-lock.yaml`)
- Workspace: `solar-monitoring-apps/pnpm-workspace.yaml` exists (minimal config)
## Frameworks
- Next.js `16.2.4` (`solar-monitoring-apps/package.json`)
- React `19.2.4` (`solar-monitoring-apps/package.json`)
- React DOM `19.2.4` (`solar-monitoring-apps/package.json`)
- Tailwind CSS `^4` (`solar-monitoring-apps/package.json`)
- ESLint `^9` (`solar-monitoring-apps/package.json`)
- `eslint-config-next` `16.2.4` (`solar-monitoring-apps/package.json`)
- Config: `solar-monitoring-apps/eslint.config.mjs` (uses `eslint-config-next/core-web-vitals` + `eslint-config-next/typescript`)
- Not detected (no Jest/Vitest/Playwright/Cypress deps or configs found)
## Key Dependencies
- `next` `16.2.4` - routing, SSR/RSC runtime
- `react` `19.2.4` / `react-dom` `19.2.4` - UI runtime
- Not detected (no API clients, DB clients, cache clients)
## Configuration
- Strict mode enabled (`"strict": true`) in `solar-monitoring-apps/tsconfig.json`
- Path alias: `@/*` → `./*` (`solar-monitoring-apps/tsconfig.json`)
- Present but empty: `solar-monitoring-apps/next.config.ts`
- `.env*` are ignored (`solar-monitoring-apps/.gitignore`)
- No usage detected in code (no `process.env.*` references found)
## Platform Requirements
- Node.js + pnpm (implied by `pnpm-lock.yaml` + scripts in `package.json`)
- `node_modules/` currently exists in repo working tree (directory present)
- Not specified (no deployment config detected; Next defaults apply)
## Gaps vs. target: “realtime Influx dashboard”
<!-- GSD:stack-end -->

<!-- GSD:conventions-start source:CONVENTIONS.md -->
## Conventions

Conventions not yet established. Will populate as patterns emerge during development.
<!-- GSD:conventions-end -->

<!-- GSD:architecture-start source:ARCHITECTURE.md -->
## Architecture

## Runtime Model
- Root layout: `app/layout.tsx`
- Root page: `app/page.tsx`
- Global styles: `app/globals.css`
- No `"use client"` directives detected in this codebase (only server components so far).
- UI currently renders static content; no client state, no data fetching logic detected.
## Modules / Components
- `next/image` in `app/page.tsx`
- `next/font/google` in `app/layout.tsx`
## Data Flow
- No API calls (`fetch`, SWR, TanStack Query, axios) detected.
- No server actions detected.
- No persistence layer detected.
## API Routes
## Configuration & Environment Variables
## Styling / UI System
## Entry Points
## Observability / Error Handling
## Implications for “realtime Influx dashboard”
- No API surface for data access (no route handlers in `app/api/*`).
- No InfluxDB client or query layer (no `@influxdata/influxdb-client*` dependency).
- No realtime transport layer (no WebSocket/SSE/Socket.IO/Pusher/Ably deps; no implementation).
- No dashboard UI architecture yet (no route structure beyond `/` and no component library).
- Desired auth/session model (no auth code/config present).
- Where secrets will live (env approach exists via `.gitignore` but not yet used in code).
<!-- GSD:architecture-end -->

<!-- GSD:skills-start source:skills/ -->
## Project Skills

No project skills found. Add skills to any of: `.claude/skills/`, `.agents/skills/`, `.cursor/skills/`, or `.github/skills/` with a `SKILL.md` index file.
<!-- GSD:skills-end -->

<!-- GSD:workflow-start source:GSD defaults -->
## GSD Workflow Enforcement

Before using Edit, Write, or other file-changing tools, start work through a GSD command so planning artifacts and execution context stay in sync.

Use these entry points:
- `/gsd-quick` for small fixes, doc updates, and ad-hoc tasks
- `/gsd-debug` for investigation and bug fixing
- `/gsd-execute-phase` for planned phase work

Do not make direct repo edits outside a GSD workflow unless the user explicitly asks to bypass it.
<!-- GSD:workflow-end -->



<!-- GSD:profile-start -->
## Developer Profile

> Profile not yet configured. Run `/gsd-profile-user` to generate your developer profile.
> This section is managed by `generate-claude-profile` -- do not edit manually.
<!-- GSD:profile-end -->

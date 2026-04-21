# Architecture (solar-monitoring-apps)

**Analysis Date:** 2026-04-21  
**Scope:** `D:/Kirei/Solar Panel Monitoring System/solar-monitoring-apps` only

## Runtime Model

**Next.js routing:** App Router (detected `app/` directory)  
- Root layout: `app/layout.tsx`
- Root page: `app/page.tsx`
- Global styles: `app/globals.css`

**Server/client split:**
- No `"use client"` directives detected in this codebase (only server components so far).
- UI currently renders static content; no client state, no data fetching logic detected.

## Modules / Components

**Current state:** no `src/`, `components/`, or `lib/` trees detected; no reusable components or domain modules detected.

**Built-in Next primitives used:**
- `next/image` in `app/page.tsx`
- `next/font/google` in `app/layout.tsx`

## Data Flow

**Current flow:** request → server render → static HTML/JS output  
- No API calls (`fetch`, SWR, TanStack Query, axios) detected.
- No server actions detected.
- No persistence layer detected.

## API Routes

**App Router route handlers:** Not detected (`app/api/**/route.ts` absent).  
**Pages Router API routes:** Not detected (`pages/api/**` absent).

## Configuration & Environment Variables

**Next config:** `next.config.ts` exists but contains no active options.  
**TypeScript:** configured via `tsconfig.json` with path alias `@/*` → `./*`.  
**Env files:** `.env*` are gitignored by `solar-monitoring-apps/.gitignore`; actual env files were not inspected (not present in repo scan).

**Runtime env usage:** no `process.env.*` usage detected in application code.

## Styling / UI System

**Tailwind:** enabled via PostCSS plugin `@tailwindcss/postcss` (`postcss.config.mjs`) and `@import "tailwindcss";` (`app/globals.css`).  
**Theme:** basic CSS variables with `prefers-color-scheme` in `app/globals.css`.

## Entry Points

**App entry:** `app/layout.tsx` (HTML shell + global font variables)  
**Home route:** `app/page.tsx` (template landing content)

## Observability / Error Handling

**Error boundaries:** Not detected (`app/**/error.tsx` absent).  
**Loading UI:** Not detected (`app/**/loading.tsx` absent).  
**Not found:** Not detected (`app/**/not-found.tsx` absent).  
**Logging:** Not detected (no logger usage; no `console.*` usage detected in app code).

## Implications for “realtime Influx dashboard”

**Blockers in current codebase (what’s missing today):**
- No API surface for data access (no route handlers in `app/api/*`).
- No InfluxDB client or query layer (no `@influxdata/influxdb-client*` dependency).
- No realtime transport layer (no WebSocket/SSE/Socket.IO/Pusher/Ably deps; no implementation).
- No dashboard UI architecture yet (no route structure beyond `/` and no component library).

**Known unknowns:**
- Desired auth/session model (no auth code/config present).
- Where secrets will live (env approach exists via `.gitignore` but not yet used in code).

---

*Architecture map: 2026-04-21*


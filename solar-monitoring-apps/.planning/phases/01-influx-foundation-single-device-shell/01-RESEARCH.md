# Phase 1: Influx Foundation + Single-Device Shell - Research

**Researched:** 2026-04-21  
**Domain:** Next.js App Router + InfluxDB v2 (Flux queries) foundation  
**Confidence:** MEDIUM

## User Constraints (from CONTEXT.md)

### Locked Decisions

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

### Claude's Discretion
- (Tidak ada bagian “Claude’s Discretion” eksplisit di `01-CONTEXT.md`.) [VERIFIED: codebase]

### Deferred Ideas (OUT OF SCOPE)

## Deferred Ideas

- MQTT status panel (removed from scope).

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| INFLUX-01 | Server bisa query InfluxDB v2 menggunakan `@influxdata/influxdb-client` (token via env, server-side only) | Use `@influxdata/influxdb-client` `InfluxDB` + `getQueryApi(org)` + `collectRows()` server-side only; route handlers/server components patterns to keep secrets server-only. [CITED: https://github.com/influxdata/influxdb-client-js/blob/master/examples/README.md] [CITED: https://github.com/vercel/next.js/blob/v16.2.2/docs/01-app/02-guides/environment-variables.mdx] |
| INFLUX-02 | Bisa ambil “latest point” untuk `device_id` tertentu (measurement `pv_monitoring`) | Flux pattern: `range()` + `filter()` + `last()` + `pivot()` to return one row with field columns; then normalize server JSON response. `pivot()` semantics verified. [CITED: https://docs.influxdata.com/flux/v0/stdlib/universe/pivot/] |
| UI-01 | Halaman utama menampilkan KPI cards (latest values) untuk MPPT & inverter | Minimal dashboard route + server fetch via internal endpoint; client renders KPI cards with loading/error states. [ASSUMED] |
| UI-03 | User bisa memilih device (by `device_id`) dan time range (preset + custom) | In Phase 1, implement UI controls + plumb to server endpoint; validate query params (Zod) and enforce defaults (env default device_id, 24h preset). [ASSUMED] |
| OPS-01 | Konfigurasi via `.env.local` (Influx URL/org/bucket/token, default device_id, refresh interval) | Use `@t3-oss/env-nextjs` `createEnv` with Zod schemas to fail fast with actionable error; `.env.local` already gitignored. [CITED: https://github.com/t3-oss/t3-env/blob/main/docs/src/app/docs/nextjs/page.mdx] |
| OPS-02 | Tidak ada secret (token) yang terkirim ke browser bundle | Keep Influx client instantiation in server-only module; never pass token to client; use Next.js Route Handlers / Server Components and optionally `import 'server-only'` guard. [CITED: https://github.com/vercel/next.js/blob/v16.2.2/docs/01-app/02-guides/environment-variables.mdx] [CITED: https://github.com/vercel/next.js/blob/v16.2.2/docs/01-app/01-getting-started/05-server-and-client-components.mdx] |
</phase_requirements>

## Summary

Phase 1 harus menghasilkan: (1) **query layer InfluxDB v2 yang aman** (token server-side only) dan (2) **UI shell single-device** yang bisa menampilkan KPI “latest values” untuk 1 `device_id` + kontrol dasar untuk memilih device dan time range (default 24h), tanpa historical charts penuh dan tanpa SSE.

Kunci planning: pisahkan **server-only Influx module** (membuat client + flux builders + normalization), **route handler** yang menjadi satu-satunya API surface untuk client, dan **env validation** yang fail-fast (startup/first request) agar error kredensial Influx jelas dan tidak “silent null UI”.

**Primary recommendation:** Use `@influxdata/influxdb-client` + Route Handler (`app/api/.../route.ts`) backed by a `server-only` Influx module, and validate env via `@t3-oss/env-nextjs` (Zod) to guarantee no secrets leak and errors are actionable. [CITED: https://github.com/influxdata/influxdb-client-js/blob/master/examples/README.md] [CITED: https://github.com/t3-oss/t3-env/blob/main/docs/src/app/docs/nextjs/page.mdx]

## Project Constraints (from .cursor/rules/)

No `.cursor/rules/` directory found in this repo. [VERIFIED: codebase]

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| `next` | 16.2.4 (existing) | App Router + Route Handlers + Server Components | Existing project baseline. [VERIFIED: codebase] |
| `@influxdata/influxdb-client` | 1.35.0 | Server-side InfluxDB v2 client for Flux queries | Official JS client; provides `InfluxDB` + `getQueryApi()` + `collectRows()` and streaming APIs. [VERIFIED: npm registry] [CITED: https://github.com/influxdata/influxdb-client-js/blob/master/examples/README.md] |
| `@t3-oss/env-nextjs` | 0.13.11 | Typesafe env validation; prevents server secrets on client | Standard pattern for Next env validation; separates server vs client env. [VERIFIED: npm registry] [CITED: https://github.com/t3-oss/t3-env/blob/main/docs/src/app/docs/nextjs/page.mdx] |
| `zod` | 4.3.6 | Validation for env + request params | Strong TS inference; `safeParse` and coercion utilities. [VERIFIED: npm registry] [CITED: https://github.com/colinhacks/zod/blob/v4.0.1/packages/docs/content/basics.mdx] |

### Supporting (Validation/Test infra to add because nyquist_validation=true)
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| `vitest` | 4.1.4 | Unit tests for server modules (env parsing, flux query builder, response normalization) | Add in Wave 0 because repo currently has no test framework. [VERIFIED: npm registry] |
| `jsdom` | 29.0.2 | DOM env for component tests (optional in Phase 1) | Only if testing UI components; otherwise keep Phase 1 tests server-only. [VERIFIED: npm registry] |
| `@testing-library/react` | 16.3.2 | React component tests (optional) | Only if Phase 1 plan includes UI tests. [VERIFIED: npm registry] |
| `@testing-library/jest-dom` | 6.9.1 | DOM matchers (optional) | Only with component tests. [VERIFIED: npm registry] |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| `@t3-oss/env-nextjs` | Hand-rolled `process.env` parsing | Higher risk of missing env at runtime; harder to guarantee “server-only” separation and actionable errors. [ASSUMED] |
| Flux `collectRows()` | `queryRows()` streaming callbacks | `queryRows()` gives more control over streaming/large result sets; `collectRows()` is simplest for small “latest KPI” payload. [CITED: https://github.com/influxdata/influxdb-client-js/blob/master/examples/README.md] |

**Installation (Phase 1 expected):**

```bash
pnpm add @influxdata/influxdb-client @t3-oss/env-nextjs zod
pnpm add -D vitest jsdom @testing-library/react @testing-library/jest-dom
```

**Version verification (done):**
- `@influxdata/influxdb-client@1.35.0` (modified 2025-11-21) [VERIFIED: npm registry]
- `@t3-oss/env-nextjs@0.13.11` (modified 2026-03-22) [VERIFIED: npm registry]
- `zod@4.3.6` (modified 2026-01-25) [VERIFIED: npm registry]
- `vitest@4.1.4` (modified 2026-04-09) [VERIFIED: npm registry]

## Architecture Patterns

### Recommended Project Structure

```
app/
├── api/
│   └── kpi/
│       └── latest/
│           └── route.ts         # GET latest KPI for device_id (+ range preset)
├── dashboard/
│   └── page.tsx                 # UI shell: KPI cards + selectors
└── page.tsx                     # redirect/entry to dashboard (optional)
lib/
├── env.ts                       # createEnv() schema
├── influx/
│   ├── client.ts                # InfluxDB({url, token}) + getQueryApi()
│   ├── queries.ts               # flux query builders (latest KPI)
│   └── normalize.ts             # normalize collectRows() to stable JSON DTOs
components/
└── kpi/
    ├── KpiCard.tsx
    └── KpiGrid.tsx
```

### Pattern 1: “Server-only Influx client module”
**What:** Create a dedicated module that instantiates `InfluxDB` using server env vars and exports helper functions. Guard it with `import 'server-only'` so accidental client imports fail. [CITED: https://github.com/vercel/next.js/blob/v16.2.2/docs/01-app/02-guides/authentication.mdx]

**When to use:** Always—anywhere that touches `INFLUX_TOKEN` or executes queries.

**Example:**

```typescript
// Source: Influx client example + Next.js server-only guard
// https://github.com/influxdata/influxdb-client-js/blob/master/examples/README.md
// https://github.com/vercel/next.js/blob/v16.2.2/docs/01-app/02-guides/authentication.mdx
import "server-only";
import { InfluxDB } from "@influxdata/influxdb-client";
import { env } from "@/lib/env";

export const influx = new InfluxDB({ url: env.INFLUX_URL, token: env.INFLUX_TOKEN });
export const queryApi = influx.getQueryApi(env.INFLUX_ORG);
```

### Pattern 2: “Route Handler is the only client-facing query surface”
**What:** Implement `GET app/api/kpi/latest/route.ts` which validates query params (`device_id`, `range`) and returns a normalized JSON payload. Route Handler runs server-side and can read private env vars. [CITED: https://github.com/vercel/next.js/blob/v16.2.2/docs/01-app/02-guides/environment-variables.mdx]

**When to use:** For any client data consumption in Phase 1.

### Pattern 3: “Fail-fast env validation”
**What:** Centralize env schema in `lib/env.ts` using `createEnv`. It should validate at startup/first import and produce actionable errors if missing/invalid. [CITED: https://github.com/t3-oss/t3-env/blob/main/docs/src/app/docs/nextjs/page.mdx]

**When to use:** Before any Influx query module is imported.

### Anti-Patterns to Avoid
- **Importing Influx client/env module into client components:** risks bundling secrets or runtime errors; keep the data access strictly in server modules/route handlers. [CITED: https://github.com/vercel/next.js/blob/v16.2.2/docs/01-app/02-guides/environment-variables.mdx]
- **Accepting Influx connection params from client input:** SSRF + credential leakage risk; only allow `device_id` + range constraints and validate them. [VERIFIED: codebase] (reinforced by `STATE.md` guardrail)
- **Using Edge runtime for Influx queries:** Node client dependencies may not be compatible with Edge; enforce Node runtime for route handlers that use Node libraries. [ASSUMED]

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Env validation | ad-hoc `process.env` checks scattered | `@t3-oss/env-nextjs` + `zod` | Centralized, typed, fail-fast; prevents server vars on client. [CITED: https://github.com/t3-oss/t3-env/blob/main/docs/src/app/docs/nextjs/page.mdx] |
| Request param validation | manual parsing with implicit defaults | `zod.safeParse` / `z.coerce.*` | Better error reporting + avoids edge-case coercion bugs. [CITED: https://github.com/colinhacks/zod/blob/v4.0.1/packages/docs/content/basics.mdx] |
| Influx query plumbing | raw HTTP to Influx API | `@influxdata/influxdb-client` | Handles query APIs and streaming patterns; standard client. [CITED: https://github.com/influxdata/influxdb-client-js/blob/master/examples/README.md] |

**Key insight:** the “hard parts” here are *boundaries* (server-only secrets) and *error surfaces* (clear env/query failures). Using standard libs makes these boundaries enforceable and testable. [ASSUMED]

## Common Pitfalls

### Pitfall 1: Secret token accidentally ends up in client bundle
**What goes wrong:** importing env/influx modules into client components or exporting env values into props can leak tokens or break builds.  
**Why it happens:** shared modules are imported from both server and client code paths.  
**How to avoid:** keep Influx code in `lib/influx/*` guarded with `import "server-only"` and only call it from Route Handlers / Server Components. [CITED: https://github.com/vercel/next.js/blob/v16.2.2/docs/01-app/02-guides/authentication.mdx]  
**Warning signs:** bundle analyzer shows env strings; code review sees `NEXT_PUBLIC_*` secrets.

### Pitfall 2: “Latest KPI” Flux query returns multiple rows / mismatched timestamps
**What goes wrong:** calling `last()` without grouping/pivoting can produce multiple rows (one per field) or mixed times.  
**Why it happens:** In Flux, fields are stored vertically (`_field`, `_value`) and `pivot()` is needed to align fields horizontally. [CITED: https://docs.influxdata.com/flux/v0/stdlib/universe/pivot/]  
**How to avoid:** use `group(columns: ["device_id", "_field"]) |> last()` and then `pivot(rowKey: ["_time","device_id"], columnKey: ["_field"], valueColumn: "_value")` (adapt as needed). [ASSUMED]

### Pitfall 3: Unhelpful env errors (blank UI)
**What goes wrong:** missing/invalid env vars cause runtime errors that surface as 500 without guidance.  
**Why it happens:** env vars are used directly in many files; errors aren’t normalized.  
**How to avoid:** validate env once (createEnv), and in Route Handler map errors to JSON response with actionable message for local dev. [CITED: https://github.com/t3-oss/t3-env/blob/main/docs/src/app/docs/nextjs/page.mdx] [ASSUMED]

## Code Examples

### Query via `collectRows()` (server-side)

```typescript
// Source: https://github.com/influxdata/influxdb-client-js/blob/master/examples/README.md
import { InfluxDB } from "@influxdata/influxdb-client";

const client = new InfluxDB({ url: "http://localhost:8086", token: process.env.TOKEN });
const queryApi = client.getQueryApi("my-org");

const result = await queryApi.collectRows(
  `from(bucket: "my-bucket") |> range(start: -1h) |> filter(fn: (r) => r._measurement == "cpu")`
);
```

### Env schema with server/client split

```typescript
// Source: https://github.com/t3-oss/t3-env/blob/main/docs/src/app/docs/nextjs/page.mdx
import { createEnv } from "@t3-oss/env-nextjs";
import { z } from "zod";

export const env = createEnv({
  server: {
    INFLUX_URL: z.string().url(),
    INFLUX_ORG: z.string().min(1),
    INFLUX_BUCKET: z.string().min(1),
    INFLUX_TOKEN: z.string().min(1),
    DEFAULT_DEVICE_ID: z.string().min(1),
  },
  client: {},
  runtimeEnv: {
    INFLUX_URL: process.env.INFLUX_URL,
    INFLUX_ORG: process.env.INFLUX_ORG,
    INFLUX_BUCKET: process.env.INFLUX_BUCKET,
    INFLUX_TOKEN: process.env.INFLUX_TOKEN,
    DEFAULT_DEVICE_ID: process.env.DEFAULT_DEVICE_ID,
  },
});
```

### Flux `pivot()` semantics (why we pivot)

```flux
// Source: https://docs.influxdata.com/flux/v0/stdlib/universe/pivot/
data
  |> pivot(rowKey: ["_time"], columnKey: ["_field"], valueColumn: "_value")
```

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | Phase 1 should add Vitest-based tests as Wave 0 gaps because nyquist_validation is enabled. | Standard Stack / Validation Architecture | If team doesn’t want tests yet, plan scope must adjust and verification becomes manual-only. |
| A2 | Best Flux pattern for “latest KPI across multiple fields” is `group(..., "_field") |> last() |> pivot(...)`. | Common Pitfalls / Phase Requirements | If schema differs (fields/tags), query must change; wrong query could return wrong KPIs. |
| A3 | Route Handlers that use `@influxdata/influxdb-client` should force Node runtime (not Edge). | Architecture Anti-Patterns | If Edge is required later, need different approach (proxy/service). |

## Open Questions (RESOLVED)

1. **Influx schema confirmation**
   - **Resolution (Phase 1)**: KPI response format will be resilient to schema variance:
     - Define a small **preferred KPI field list** (for PV/inverter) in server code.
     - Query “latest” data using Flux that returns the latest values available.
     - In the response, return:
       - `deviceId`
       - `time` (last timestamp observed)
       - `values: Record<string, number | string | boolean | null>` for the preferred fields (missing fields become `null`)
       - `availableFields: string[]` (fields actually present in the last row) to aid debugging.
   - **Why**: avoids blocking Phase 1 on perfect schema knowledge; Phase 2 can refine chart field set after schema is confirmed.

2. **Device picker source**
   - **Resolution (Phase 1)**: implement device selector as **manual text input** with default pre-filled from env (e.g. `DEFAULT_DEVICE_ID`).
   - **No distinct-tag query** in Phase 1 (avoid extra endpoints/scope). Phase 2+ can add device discovery if needed.

3. **Time range “custom” definition**
   - **Resolution (Phase 1)**: only support **preset** time range (minimum `24h`, default).
   - API accepts `rangePreset` enum (e.g. `"24h"`) validated server-side; no custom start/stop in Phase 1 (explicitly deferred to Phase 2).

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| Node.js | Next.js runtime + scripts | ✓ | v22.22.0 | — |
| pnpm | install/run | ✓ | 10.29.3 | npm (not preferred) |
| npm registry access | install deps | ✓ | npm 10.9.4 | offline mirror (if needed) |
| InfluxDB reachable | INFLUX-01/02 | ? | — | None (blocking if unavailable) |

**Missing dependencies with no fallback:**
- Influx credentials/reachability (must be provided to validate end-to-end query). [VERIFIED: STATE.md]

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | Vitest (recommended add) [VERIFIED: npm registry] |
| Config file | none (gap) |
| Quick run command | `pnpm test` (to be added) |
| Full suite command | `pnpm test` + `pnpm lint` (to be added) |

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| INFLUX-01 | Influx client constructs QueryApi using env and can run a simple query | unit (module) | `pnpm test -t influx` | ❌ Wave 0 |
| INFLUX-02 | “latest KPI” query builder yields Flux that includes measurement + device filter + last/pivot | unit (pure) | `pnpm test -t latestKpiFlux` | ❌ Wave 0 |
| OPS-01 | Missing env fails with actionable error | unit | `pnpm test -t env` | ❌ Wave 0 |
| OPS-02 | API payload has no token and route imports server-only modules only | unit/smoke | `pnpm test -t noSecret` | ❌ Wave 0 |
| UI-01 | KPI cards render given DTO | unit (component) | `pnpm test -t KpiCard` | ❌ Wave 0 (optional) |
| UI-03 | Selector updates request params (device/range) | unit (component) | `pnpm test -t selectors` | ❌ Wave 0 (optional) |

### Wave 0 Gaps
- [ ] Add Vitest + minimal config + `pnpm test` script.
- [ ] Add first unit tests for env parsing and flux query builder/normalizer.

## Security Domain

### Applicable ASVS Categories
| ASVS Category | Applies | Standard Control |
|--------------|---------|------------------|
| V2 Authentication | no (v1) | Out of scope (LAN/internal) [VERIFIED: REQUIREMENTS.md] |
| V3 Session Management | no (v1) | — |
| V4 Access Control | partial | Route Handler must not accept Influx credentials from client; validate allowed inputs only. [VERIFIED: STATE.md] |
| V5 Input Validation | yes | `zod` for query params and env. [CITED: https://github.com/colinhacks/zod/blob/v4.0.1/packages/docs/content/basics.mdx] |
| V6 Cryptography | no | TLS handled by Influx endpoint/proxy (deployment concern). [ASSUMED] |

### Known Threat Patterns for this phase
| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| Secret leakage (token in client) | Information disclosure | Server-only modules + no `NEXT_PUBLIC_*` secrets; route handlers only. [VERIFIED: CONTEXT.md] |
| SSRF / internal network access | Tampering/Info disclosure | Do not accept URL/org/bucket/token from client input; only use server env. [VERIFIED: STATE.md] |
| Injection into Flux query | Tampering | Strictly validate `device_id` and allowed range presets; avoid string concatenation with untrusted input where possible. [ASSUMED] |

## Sources

### Primary (HIGH confidence)
- `@influxdata/influxdb-client` examples (QueryApi, `collectRows`, `queryRows`) [CITED: https://github.com/influxdata/influxdb-client-js/blob/master/examples/README.md]
- Next.js App Router environment vars + server-only patterns [CITED: https://github.com/vercel/next.js/blob/v16.2.2/docs/01-app/02-guides/environment-variables.mdx] [CITED: https://github.com/vercel/next.js/blob/v16.2.2/docs/01-app/01-getting-started/05-server-and-client-components.mdx] [CITED: https://github.com/vercel/next.js/blob/v16.2.2/docs/01-app/02-guides/authentication.mdx]
- T3 Env Next.js env validation docs [CITED: https://github.com/t3-oss/t3-env/blob/main/docs/src/app/docs/nextjs/page.mdx]
- Flux `pivot()` function docs [CITED: https://docs.influxdata.com/flux/v0/stdlib/universe/pivot/]

### Secondary (MEDIUM confidence)
- npm registry metadata for package versions/dates (captured via `npm view`) [VERIFIED: npm registry]

### Tertiary (LOW confidence)
- Flux “latest per field” composition advice from community threads (used only as a starting point; must verify against actual schema) [ASSUMED]

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — versions verified from npm; core docs cited. [VERIFIED: npm registry]
- Architecture: MEDIUM — Next.js server-only patterns cited; exact folder layout is recommended but unverified in codebase. [VERIFIED: codebase] [CITED: next.js docs]
- Pitfalls: MEDIUM — security pitfalls verified by constraints; Flux query composition partly assumed until schema is confirmed.

**Research date:** 2026-04-21  
**Valid until:** 2026-05-21


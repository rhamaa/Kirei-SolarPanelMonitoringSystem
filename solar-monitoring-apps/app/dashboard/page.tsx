import type { Metadata } from "next";

import { DashboardClient } from "@/components/dashboard/DashboardClient";

export const metadata: Metadata = {
  title: "Dashboard",
};

const REQUIRED_ENV_VARS = [
  "INFLUX_URL",
  "INFLUX_ORG",
  "INFLUX_BUCKET",
  "INFLUX_TOKEN",
  "DEFAULT_DEVICE_ID",
] as const;

function ConfigMissingCallout() {
  return (
    <div className="w-full rounded-2xl border border-red-200 bg-red-50 px-5 py-4 text-red-950 dark:border-red-900/50 dark:bg-red-950/25 dark:text-red-100">
      <h2 className="text-lg font-semibold leading-6">Influx isn’t configured</h2>
      <p className="mt-1 text-sm leading-6">
        Influx isn’t configured for this app. Add the required variables to{" "}
        <code className="rounded bg-red-900/10 px-1.5 py-0.5 text-[0.95em] dark:bg-red-50/10">
          .env.local
        </code>
        , restart the dev server, then retry.
      </p>

      <ul className="mt-3 list-disc space-y-1 pl-5 text-sm leading-6">
        {REQUIRED_ENV_VARS.map((name) => (
          <li key={name}>
            <code className="rounded bg-red-900/10 px-1.5 py-0.5 text-[0.95em] dark:bg-red-50/10">
              {name}
            </code>
          </li>
        ))}
      </ul>
    </div>
  );
}

export default async function DashboardPage() {
  let defaultDeviceId: string | null = null;
  let defaultRange: "1h" | "6h" | "24h" | "7d" = "24h";

  try {
    const { env } = await import("@/lib/env");
    defaultDeviceId = env.DEFAULT_DEVICE_ID;
    defaultRange = env.KPI_RANGE_DEFAULT;
  } catch {
    defaultDeviceId = null;
  }

  return (
    <div className="flex flex-1 justify-center bg-background px-4">
      <div className="w-full max-w-[1024px] py-8">
        <div className="sticky top-0 z-10 -mx-4 mb-6 border-b border-black/5 bg-background/80 px-4 py-4 backdrop-blur dark:border-white/10">
          <div className="flex flex-col gap-3 sm:flex-row sm:items-center sm:justify-between">
            <h1 className="text-xl font-semibold tracking-tight">Dashboard</h1>
            <div className="h-11 rounded-xl bg-zinc-100 px-3 dark:bg-zinc-900" />
          </div>
        </div>

        {defaultDeviceId ? (
          <DashboardClient defaultDeviceId={defaultDeviceId} defaultRange={defaultRange} />
        ) : (
          <ConfigMissingCallout />
        )}
      </div>
    </div>
  );
}


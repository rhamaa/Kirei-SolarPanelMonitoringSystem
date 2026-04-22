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
    <div
      className="w-full max-w-[720px] rounded-2xl border px-5 py-4"
      style={{ borderColor: "rgba(255,94,94,0.35)", background: "rgba(255,94,94,0.08)", color: "var(--text)" }}
    >
      <h2 className="text-lg font-bold leading-6" style={{ fontFamily: "var(--font-app-mono)" }}>
        Influx isn’t configured
      </h2>
      <p className="mt-1 text-sm leading-6" style={{ color: "var(--muted)" }}>
        Add the required variables to{" "}
        <code className="rounded bg-white/5 px-1.5 py-0.5 text-[0.95em]" style={{ fontFamily: "var(--font-app-mono)" }}>
          .env.local
        </code>
        , restart the dev server, then retry.
      </p>

      <ul className="mt-3 list-disc space-y-1 pl-5 text-sm leading-6" style={{ color: "var(--muted)" }}>
        {REQUIRED_ENV_VARS.map((name) => (
          <li key={name}>
            <code className="rounded bg-white/5 px-1.5 py-0.5 text-[0.95em]" style={{ fontFamily: "var(--font-app-mono)" }}>
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
    <div className="flex flex-1 justify-center bg-background">
      <div className="w-full py-0">
        {defaultDeviceId ? (
          <DashboardClient defaultDeviceId={defaultDeviceId} defaultRange={defaultRange} />
        ) : (
          <div className="mx-auto max-w-[1200px] px-4 py-10 md:px-7">
            <ConfigMissingCallout />
          </div>
        )}
      </div>
    </div>
  );
}


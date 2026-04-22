"use client";

type RangePreset = "1h" | "6h" | "24h" | "7d";

export function DashboardClient(props: {
  defaultDeviceId: string;
  defaultRange: RangePreset;
}) {
  const { defaultDeviceId, defaultRange } = props;

  return (
    <div className="rounded-2xl border border-black/5 bg-zinc-50 px-5 py-4 dark:border-white/10 dark:bg-zinc-950">
      <div className="flex flex-col gap-2 sm:flex-row sm:items-center sm:justify-between">
        <div className="text-sm font-semibold">Device</div>
        <div className="text-sm text-zinc-700 dark:text-zinc-300">
          <code className="rounded bg-black/5 px-1.5 py-0.5 dark:bg-white/10">
            {defaultDeviceId}
          </code>{" "}
          <span className="text-zinc-500 dark:text-zinc-400">·</span>{" "}
          <span className="text-zinc-600 dark:text-zinc-300">{defaultRange}</span>
        </div>
      </div>
      <p className="mt-4 text-sm text-zinc-600 dark:text-zinc-400">
        KPI cards will appear here after the dashboard client is fully wired.
      </p>
    </div>
  );
}


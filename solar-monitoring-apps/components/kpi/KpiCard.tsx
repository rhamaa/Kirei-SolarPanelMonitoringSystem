type KpiCardState =
  | { state: "loading" }
  | { state: "ready"; value: number | null; unit: string; time: string | null };

function formatValue(value: number | null) {
  if (value === null) return "—";
  const abs = Math.abs(value);
  if (abs >= 1000) return value.toFixed(0);
  if (abs >= 100) return value.toFixed(1);
  if (abs >= 10) return value.toFixed(2);
  return value.toFixed(3);
}

function formatTime(time: string | null) {
  if (!time) return "Last updated —";
  const d = new Date(time);
  if (Number.isNaN(d.getTime())) return "Last updated —";
  return `Last updated ${d.toLocaleString()}`;
}

export function KpiCard(props: { label: string; data: KpiCardState }) {
  const { label, data } = props;

  return (
    <div className="rounded-2xl border border-black/5 bg-zinc-50 px-5 py-4 dark:border-white/10 dark:bg-zinc-950">
      <div className="text-sm font-semibold text-zinc-900 dark:text-zinc-100">{label}</div>

      <div className="mt-3 flex items-baseline gap-2">
        {data.state === "loading" ? (
          <div className="h-8 w-28 animate-pulse rounded-lg bg-zinc-200 dark:bg-zinc-800" />
        ) : (
          <>
            <div className="text-[28px] font-semibold leading-none tracking-tight text-zinc-950 dark:text-zinc-50">
              {formatValue(data.value)}
            </div>
            <div className="text-sm font-semibold text-zinc-600 dark:text-zinc-300">
              {data.unit}
            </div>
          </>
        )}
      </div>

      <div className="mt-3 text-sm leading-6 text-zinc-600 dark:text-zinc-400">
        {data.state === "loading" ? (
          <div className="h-5 w-44 animate-pulse rounded bg-zinc-200 dark:bg-zinc-800" />
        ) : (
          formatTime(data.time)
        )}
      </div>
    </div>
  );
}


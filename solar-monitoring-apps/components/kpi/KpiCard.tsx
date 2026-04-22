type KpiCardState =
  | { state: "loading" }
  | {
      state: "ready";
      value: number | null;
      unit: string;
      time: string | null;
      tone?: "accent" | "cyan" | "green" | "muted";
      glow?: boolean;
    };

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

  const tone = data.state === "ready" ? data.tone : undefined;
  const toneColor =
    tone === "cyan"
      ? "var(--cyan)"
      : tone === "green"
        ? "var(--green)"
        : tone === "muted"
          ? "var(--muted)"
          : "var(--accent)";

  return (
    <div
      className="relative overflow-hidden rounded-xl border px-[18px] py-4"
      style={{
        background: "var(--bg2)",
        borderColor: data.state === "ready" && data.glow ? toneColor : "var(--border)",
        boxShadow: data.state === "ready" && data.glow ? `0 0 20px ${toneColor}33` : "none",
      }}
    >
      <div className="font-mono text-[11px] font-semibold uppercase tracking-[0.12em]" style={{ color: "var(--muted)" }}>
        {label}
      </div>

      <div className="mt-3 flex items-baseline gap-2">
        {data.state === "loading" ? (
          <div className="h-8 w-28 animate-pulse rounded-lg" style={{ background: "rgba(255,255,255,0.08)" }} />
        ) : (
          <>
            <div className="font-mono text-[28px] font-bold leading-none tracking-tight" style={{ color: toneColor }}>
              {formatValue(data.value)}
            </div>
            <div className="font-mono text-xs font-semibold" style={{ color: "var(--muted)" }}>
              {data.unit}
            </div>
          </>
        )}
      </div>

      <div className="mt-3 text-[11px] leading-6" style={{ color: "var(--muted)" }}>
        {data.state === "loading" ? (
          <div className="h-5 w-44 animate-pulse rounded" style={{ background: "rgba(255,255,255,0.08)" }} />
        ) : (
          formatTime(data.time)
        )}
      </div>
    </div>
  );
}


"use client";

export function clamp(v: number, lo: number, hi: number) {
  return Math.max(lo, Math.min(hi, v));
}

export function fmt(v: number | null | undefined, d = 1) {
  if (v == null || !Number.isFinite(v)) return "—";
  return Number(v).toFixed(d);
}

export function Sparkline(props: { data: number[]; color: string; height?: number; fill?: boolean }) {
  const { data, color, height = 40, fill = true } = props;
  if (!data || data.length < 2) return null;
  const w = 200;
  const h = height;
  const min = Math.min(...data);
  const max = Math.max(...data);
  const range = max - min || 1;
  const pts = data.map((v, i) => [(i / (data.length - 1)) * w, h - ((v - min) / range) * (h - 4) - 2] as const);
  const path = pts.map((p, i) => `${i === 0 ? "M" : "L"}${p[0].toFixed(1)},${p[1].toFixed(1)}`).join(" ");
  const fillPath = `${path} L${w},${h} L0,${h} Z`;
  return (
    <svg viewBox={`0 0 ${w} ${h}`} className="w-full" style={{ height }} preserveAspectRatio="none">
      {fill ? <path d={fillPath} fill={color} fillOpacity={0.12} /> : null}
      <path
        d={path}
        stroke={color}
        strokeWidth={1.5}
        fill="none"
        strokeLinecap="round"
        strokeLinejoin="round"
        style={{ filter: `drop-shadow(0 0 3px ${color}88)` }}
      />
      <circle cx={pts[pts.length - 1][0]} cy={pts[pts.length - 1][1]} r={2.5} fill={color} />
    </svg>
  );
}

export function ArcGauge(props: { value: number; max: number; color: string; size?: number }) {
  const { value, max, color, size = 80 } = props;
  const r = size / 2 - 8;
  const cx = size / 2;
  const cy = size / 2;
  const startAngle = -210;
  const endAngle = 30;
  const totalArc = endAngle - startAngle;
  const pct = clamp(value / max, 0, 1);
  const toRad = (d: number) => (d * Math.PI) / 180;
  const arc = (a: number) => [cx + r * Math.cos(toRad(a)), cy + r * Math.sin(toRad(a))] as const;
  const [sx, sy] = arc(startAngle);
  const [ex, ey] = arc(endAngle);
  const [vx, vy] = arc(startAngle + totalArc * pct);
  const largeArc = totalArc > 180 ? 1 : 0;
  const vLargeArc = totalArc * pct > 180 ? 1 : 0;
  return (
    <svg width={size} height={size} className="overflow-visible">
      <path
        d={`M${sx},${sy} A${r},${r} 0 ${largeArc},1 ${ex},${ey}`}
        stroke="rgba(255,255,255,0.07)"
        strokeWidth={5}
        fill="none"
        strokeLinecap="round"
      />
      {pct > 0.01 ? (
        <path
          d={`M${sx},${sy} A${r},${r} 0 ${vLargeArc},1 ${vx},${vy}`}
          stroke={color}
          strokeWidth={5}
          fill="none"
          strokeLinecap="round"
          style={{ filter: `drop-shadow(0 0 4px ${color})` }}
        />
      ) : null}
    </svg>
  );
}

function FlowPath(props: { x1: number; y1: number; x2: number; y2: number; active: boolean; color: string }) {
  const { x1, y1, x2, y2, active, color } = props;
  return (
    <g>
      <line x1={x1} y1={y1} x2={x2} y2={y2} stroke="rgba(255,255,255,0.06)" strokeWidth={1.5} />
      {active ? (
        <line
          x1={x1}
          y1={y1}
          x2={x2}
          y2={y2}
          stroke={color}
          strokeWidth={2}
          strokeDasharray="8 12"
          style={{ animation: "pv-flow 1.2s linear infinite" }}
        />
      ) : null}
    </g>
  );
}

function FlowNode(props: {
  label: string;
  icon: string;
  active: boolean;
  color: string;
  x: number;
  y: number;
  value: number | null;
  unit: string;
}) {
  const { label, icon, active, color, x, y, value, unit } = props;
  return (
    <g transform={`translate(${x},${y})`}>
      <circle
        r={28}
        fill={active ? color : "rgba(255,255,255,0.04)"}
        fillOpacity={active ? 0.15 : 1}
        stroke={active ? color : "rgba(255,255,255,0.1)"}
        strokeWidth={1.5}
        style={active ? { filter: `drop-shadow(0 0 8px ${color})` } : undefined}
      />
      <text
        textAnchor="middle"
        dominantBaseline="middle"
        fontSize={18}
        fill={active ? color : "rgba(255,255,255,0.3)"}
      >
        {icon}
      </text>
      <text textAnchor="middle" y={40} fontSize={9} fill="rgba(255,255,255,0.5)" fontFamily="var(--font-app-mono)" letterSpacing={1}>
        {label}
      </text>
      {value != null && Number.isFinite(value) ? (
        <text textAnchor="middle" y={53} fontSize={10} fill={active ? color : "rgba(255,255,255,0.25)"} fontFamily="var(--font-app-mono)" fontWeight={700}>
          {fmt(value)}
          <tspan fontSize={8} opacity={0.7}>
            {" "}
            {unit}
          </tspan>
        </text>
      ) : null}
    </g>
  );
}

export function EnergyFlowPanel(props: {
  pvV: number | null;
  mpptW: number | null;
  acW: number | null;
  batV: number | null;
}) {
  const { pvV, mpptW, acW, batV } = props;
  const accent = "var(--accent)";
  const cyan = "var(--cyan)";
  const green = "var(--green)";
  const mpptOn = (mpptW ?? 0) > 0.5;
  const acOn = (acW ?? 0) > 0.5;

  return (
    <div
      className="relative overflow-hidden rounded-2xl p-6 md:p-7"
      style={{ background: "var(--bg2)", border: "1px solid var(--border)", animation: "pv-fade-in 0.3s ease" }}
    >
      <div className="mb-4 flex items-center justify-between">
        <div className="text-[12px] font-bold uppercase tracking-[0.12em]" style={{ fontFamily: "var(--font-app-mono)", color: "var(--accent)" }}>
          Energy Flow
        </div>
        <div className="text-[11px]" style={{ fontFamily: "var(--font-app-mono)", color: "var(--muted)" }}>
          Real-time
        </div>
      </div>
      <div className="flex justify-center">
        <svg viewBox="0 0 520 130" className="h-[130px] w-full max-w-[520px]">
          <FlowPath x1={95} y1={55} x2={195} y2={55} active={mpptOn} color={accent} />
          <FlowPath x1={315} y1={55} x2={415} y2={55} active={acOn} color={cyan} />
          <FlowPath x1={255} y1={83} x2={255} y2={113} active={false} color={green} />

          <FlowNode label="SOLAR" icon="☀" active color={accent} x={55} y={55} value={pvV} unit="V" />
          <FlowNode label="MPPT" icon="⚡" active={mpptOn} color={accent} x={255} y={55} value={mpptW} unit="W" />
          <FlowNode label="AC OUT" icon="🔌" active={acOn} color={cyan} x={455} y={55} value={acW} unit="W" />
          <FlowNode label="BATTERY" icon="🔋" active={false} color={green} x={255} y={113} value={batV} unit="V" />
        </svg>
      </div>
    </div>
  );
}

export function StatCard(props: {
  label: string;
  value: string;
  unit: string;
  color: string;
  history?: number[];
  glow?: boolean;
  icon?: string;
}) {
  const { label, value, unit, color, history, glow, icon } = props;
  return (
    <div
      className="relative overflow-hidden rounded-xl px-[18px] py-4 transition-[border-color]"
      style={{
        background: "var(--bg2)",
        border: `1px solid ${glow ? color : "var(--border)"}`,
        animation: glow ? "pv-glow 3s ease infinite" : undefined,
        boxShadow: glow ? `0 0 20px ${color}33` : "none",
      }}
    >
      <div className="mb-2 flex items-start justify-between">
        <span className="text-[11px] font-semibold uppercase tracking-[0.1em]" style={{ color: "var(--muted)", fontFamily: "var(--font-app-mono)" }}>
          {label}
        </span>
        {icon ? <span className="text-sm opacity-60">{icon}</span> : null}
      </div>
      <div className="mb-1 flex items-baseline gap-2">
        <span className="text-[26px] font-bold leading-none" style={{ fontFamily: "var(--font-app-mono)", color }}>
          {value}
        </span>
        <span className="text-xs" style={{ color: "var(--muted)", fontFamily: "var(--font-app-mono)" }}>
          {unit}
        </span>
      </div>
      {history && history.length > 2 ? <Sparkline data={history} color={color} height={36} /> : null}
    </div>
  );
}

/** Segmented bar from Influx `battery_soc` (0–100). */
export function BatterySocBar(props: { socPercent: number | null }) {
  const { socPercent } = props;
  if (socPercent == null || !Number.isFinite(socPercent)) {
    return <div className="text-[11px]" style={{ color: "var(--muted)", fontFamily: "var(--font-app-mono)" }}>No battery_soc in Influx</div>;
  }
  const pct = clamp(socPercent / 100, 0, 1);
  const color = pct > 0.6 ? "var(--green)" : pct > 0.3 ? "var(--amber)" : "var(--red)";
  const segments = 10;
  return (
    <div>
      <div className="mb-1.5 flex gap-1">
        {Array.from({ length: segments }).map((_, i) => {
          const filled = i / segments < pct;
          return (
            <div
              key={i}
              className="h-5 flex-1 rounded-sm transition-all duration-500"
              style={{
                background: filled ? color : "rgba(255,255,255,0.05)",
                boxShadow: filled ? `0 0 6px ${color}88` : "none",
              }}
            />
          );
        })}
        <div
          className="ml-0.5 self-center rounded-r-sm"
          style={{ width: 6, height: 10, background: "rgba(255,255,255,0.15)", borderRadius: "0 2px 2px 0" }}
        />
      </div>
      <div className="flex justify-between text-[10px]" style={{ color: "var(--muted)", fontFamily: "var(--font-app-mono)" }}>
        <span>0%</span>
        <span style={{ color }}>{fmt(socPercent, 0)}%</span>
        <span>100%</span>
      </div>
    </div>
  );
}

export function PFGauge(props: { pf: number | null }) {
  const { pf } = props;
  if (pf == null || !Number.isFinite(pf)) {
    return <div className="h-[50px] w-20" style={{ background: "rgba(255,255,255,0.05)", borderRadius: 8 }} />;
  }
  const angle = (pf - 0) * 180;
  const r = 30;
  const cx = 40;
  const cy = 40;
  const toRad = (d: number) => (d * Math.PI) / 180;
  const clr = pf > 0.9 ? "var(--green)" : pf > 0.75 ? "var(--amber)" : "var(--red)";
  return (
    <svg width="80" height="50">
      <path d="M10,40 A30,30 0 0,1 70,40" stroke="rgba(255,255,255,0.07)" strokeWidth={4} fill="none" strokeLinecap="round" />
      <path
        d={`M10,40 A30,30 0 0,1 ${cx + r * Math.cos(toRad(180 - angle * 0.99))},${cy - r * Math.sin(toRad(180 - angle * 0.99))}`}
        stroke={clr}
        strokeWidth={4}
        fill="none"
        strokeLinecap="round"
        style={{ filter: `drop-shadow(0 0 4px ${clr})` }}
      />
      <line
        x1={cx}
        y1={cy}
        x2={cx + 22 * Math.cos(toRad(180 - angle * 0.99))}
        y2={cy - 22 * Math.sin(toRad(180 - angle * 0.99))}
        stroke={clr}
        strokeWidth={2}
        strokeLinecap="round"
      />
      <circle cx={cx} cy={cy} r={3} fill={clr} />
    </svg>
  );
}

export function AreaChartMini(props: { data: number[]; color: string; height?: number }) {
  const { data, color, height = 80 } = props;
  if (!data || data.length === 0) return null;
  const w = 600;
  const h = height;
  const min = 0;
  const max = Math.max(...data) * 1.15 || 1;
  const pts = data.map((v, i) => [(i / (data.length - 1)) * w, h - ((v - min) / (max - min)) * (h - 8) - 4] as const);
  const path = pts.map((p, i) => `${i === 0 ? "M" : "L"}${p[0].toFixed(1)},${p[1].toFixed(1)}`).join(" ");
  const fillPath = `${path} L${w},${h} L0,${h} Z`;
  const gid = color.replace(/[^a-zA-Z0-9]/g, "");
  const step = Math.ceil(data.length / 8);
  return (
    <svg viewBox={`0 0 ${w} ${h}`} className="w-full" style={{ height }} preserveAspectRatio="none">
      <defs>
        <linearGradient id={`pv-grad-${gid}`} x1="0" y1="0" x2="0" y2="1">
          <stop offset="0%" stopColor={color} stopOpacity={0.3} />
          <stop offset="100%" stopColor={color} stopOpacity={0.02} />
        </linearGradient>
      </defs>
      <path d={fillPath} fill={`url(#pv-grad-${gid})`} />
      <path
        d={path}
        stroke={color}
        strokeWidth={2}
        fill="none"
        strokeLinecap="round"
        strokeLinejoin="round"
        style={{ filter: `drop-shadow(0 0 4px ${color}88)` }}
      />
      {pts.map((p, i) =>
        i % step === 0 ? (
          <line key={i} x1={p[0]} y1={h - 1} x2={p[0]} y2={h - 4} stroke="rgba(255,255,255,0.15)" strokeWidth={1} />
        ) : null,
      )}
    </svg>
  );
}

export function StatusBadge(props: { text: string; ok: boolean }) {
  const { text, ok } = props;
  const color = ok ? "var(--green)" : "var(--red)";
  return (
    <span
      className="inline-flex items-center gap-1.5 rounded-full px-2.5 py-0.5 text-[10px] uppercase tracking-wide"
      style={{
        fontFamily: "var(--font-app-mono)",
        color,
        background: ok ? "var(--green-dim)" : "rgba(255,94,94,0.1)",
        border: `1px solid ${ok ? "rgba(57,217,138,0.3)" : "rgba(255,94,94,0.3)"}`,
      }}
    >
      <span className="inline-block h-[5px] w-[5px] rounded-full" style={{ background: color, animation: "pv-pulse 2s infinite" }} />
      {text}
    </span>
  );
}

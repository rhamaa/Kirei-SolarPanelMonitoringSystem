import { KpiCard } from "./KpiCard";

export type KpiGridItem =
  | { key: string; label: string; state: "loading" }
  | { key: string; label: string; state: "ready"; value: number | null; unit: string; time: string | null };

export function KpiGrid(props: { items: KpiGridItem[] }) {
  const { items } = props;

  return (
    <div className="grid grid-cols-1 gap-4 sm:grid-cols-2">
      {items.map((item) => (
        <KpiCard
          key={item.key}
          label={item.label}
          data={
            item.state === "loading"
              ? { state: "loading" }
              : { state: "ready", value: item.value, unit: item.unit, time: item.time }
          }
        />
      ))}
    </div>
  );
}


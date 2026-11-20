"""band_table.py report.md [out.md]: the bake report as tables per band of ten levels (P5 task 7) and the P5 checks.

Per band: the greedy bot's median time (in T) and core, fight and the cancelled share reached, the naive bot's loss
rate, the lowest z a gloom reached, the highest tide and unit counts, seeds tried. Checks: the naive loss rate never
falls by more than ten points from one band to the next; every level's cancelled share is within 5 points of fight.
"""
import re
import statistics
import sys


def rows(path):
    out = []
    for line in open(path, encoding="utf-8"):
        if not re.match(r"^\| \d+ \|", line):
            continue
        c = [x.strip() for x in line.strip().strip("|").split("|")]
        t = re.match(r"([\d.]+) \(([\d.]+)\)", c[10])
        out.append({
            "level": int(c[0]), "accepted": c[1] == "yes", "rows": int(c[2]), "patterns": c[3], "p0": float(c[4]),
            "fight": float(c[5].rstrip("%")), "cancelled": float(c[6].rstrip("%")), "scale": float(c[7]), "spire": int(c[8]),
            "wins": int(c[9].split("/")[0]), "time": float(t.group(1)), "T": float(t.group(2)), "core": float(c[11]),
            "naive_loses": c[12] == "loses", "lowest": float(c[13]), "tide": int(c[14]), "units": int(c[15]), "seeds": int(c[16]),
        })
    return out


def main(argv):
    data = rows(argv[0])
    lines = ["| Levels | Greedy time (T) | Greedy core | Fight | Cancelled | Naive loses | Lowest gloom z | Peak tide | Peak units | Seeds tried |",
             "|---|---|---|---|---|---|---|---|---|---|"]
    bands = {}
    for r in data:
        bands.setdefault((r["level"] - 1) // 10, []).append(r)
    rates = []
    for b in sorted(bands):
        rs = bands[b]
        lo, hi = rs[0]["level"], rs[-1]["level"]
        rate = 100.0 * sum(r["naive_loses"] for r in rs) / len(rs)
        rates.append((f"{lo}-{hi}", rate))
        lines.append(
            f"| {lo}-{hi} | {statistics.median(r['time'] / r['T'] for r in rs):.2f} | {statistics.median(r['core'] for r in rs):.0f} | "
            f"{statistics.median(r['fight'] for r in rs):.0f}% | {statistics.median(r['cancelled'] for r in rs):.0f}% "
            f"({min(r['cancelled'] for r in rs):.0f}-{max(r['cancelled'] for r in rs):.0f}) | {rate:.0f}% | "
            f"{min(r['lowest'] for r in rs):.1f} | {max(r['tide'] for r in rs)} | {max(r['units'] for r in rs)} | "
            f"{sum(r['seeds'] for r in rs)} |")
    bad_share = [r for r in data if abs(r["cancelled"] - r["fight"]) > 5.0]
    drops = [(rates[i][0], rates[i + 1][0], rates[i][1], rates[i + 1][1]) for i in range(len(rates) - 1)
             if rates[i][1] - rates[i + 1][1] > 10.0]
    not_acc = [r["level"] for r in data if not r["accepted"]]
    lines += ["", f"Levels: {len(data)}, not accepted: {len(not_acc)} {not_acc[:20]}",
              f"Cancelled share more than 5 points from fight: {len(bad_share)} "
              + ", ".join(f"{r['level']} ({r['cancelled']:.0f}% vs {r['fight']:.0f}%)" for r in bad_share[:30]),
              f"Naive loss rate falling more than 10 points between bands: {len(drops)} "
              + ", ".join(f"{a} {x:.0f}% -> {b} {y:.0f}%" for a, b, x, y in drops)]
    text = "\n".join(lines)
    print(text)
    if len(argv) > 1:
        with open(argv[1], "w", encoding="utf-8") as fh:
            fh.write("# Difficulty by band of ten levels\n\nFrom the bake report (`tools/band_table.py`).\n\n" + text + "\n")
    return 0 if not bad_share and not drops and not not_acc else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))

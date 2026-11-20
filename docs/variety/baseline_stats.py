"""baseline_stats.py [report.md]: how alike the 300 levels of 2026-11-20 are, from the bake report (VARIETY_PLAN 2).

Reads captures/levels/report.md (one row a level: rows, pattern letters, P0, fight, cancelled share, spire, greedy's
time and core, naive's result, lowest gloom z, peak tide, peak units, seeds) and prints the numbers quoted in
docs/VARIETY_PLAN.md. Run again after a bake to compare; the variety report of phase V0 replaces it with a fingerprint
taken from the level data itself (VARIETY_TECH 6).

  python -I -B docs/variety/baseline_stats.py [captures/levels/report.md]
"""
import collections
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
            "level": int(c[0]), "rows": int(c[2]), "patterns": c[3], "p0": float(c[4]), "fight": float(c[5].rstrip("%")),
            "spire": int(c[8]), "time": float(t.group(1)), "T": float(t.group(2)), "core": float(c[11]),
            "naive_loses": c[12] == "loses", "lowest": float(c[13]), "tide": int(c[14]), "units": int(c[15]),
        })
    return out


def main(argv):
    path = argv[0] if argv else "captures/levels/report.md"
    data = [r for r in rows(path) if r["level"] >= 6]  # the generated levels
    n = len(data)
    print(f"generated levels: {n} (6-{data[-1]['level']})")

    # 1. Pattern strings (one letter a row, in order from the launcher).
    by_string = collections.defaultdict(list)
    for r in data:
        by_string[r["patterns"]].append(r["level"])
    shared = sum(len(v) for v in by_string.values() if len(v) > 1)
    print(f"distinct pattern strings: {len(by_string)}; levels whose string another level also has: {shared} ({100 * shared / n:.0f}%)")
    top = sorted(by_string.items(), key=lambda kv: -len(kv[1]))[:6]
    print("most repeated strings: " + "; ".join(f"{k} x{len(v)} (levels {', '.join(map(str, v[:6]))}{'...' if len(v) > 6 else ''})" for k, v in top))
    gaps = []
    for levels in by_string.values():
        gaps += [b - a for a, b in zip(levels, levels[1:])]
    near = sum(1 for g in gaps if g <= 30)
    print(f"repeats of a string within 30 levels of its last use: {near}; within 10: {sum(1 for g in gaps if g <= 10)}")
    multisets = {"".join(sorted(r["patterns"])) for r in data}
    print(f"distinct pattern sets, order ignored: {len(multisets)}")

    # 2. What changes with the level number.
    def band(lo, hi):
        return [r for r in data if lo <= r["level"] <= hi]
    print("\n| Levels | Rows | Target T (non-boss) | Fight | Greedy time / T | Greedy core | Naive loses | Lowest gloom z (median) | Peak units (median) |")
    print("|---|---|---|---|---|---|---|---|---|")
    for lo, hi in [(6, 9), (10, 19), (20, 39), (40, 99), (100, 199), (200, 300)]:
        b = band(lo, hi)
        ts = sorted({r["T"] for r in b if r["level"] % 10})
        print(f"| {lo}-{hi} | {min(r['rows'] for r in b)}-{max(r['rows'] for r in b)} | {ts[0]:.0f}-{ts[-1]:.0f} s | "
              f"{min(r['fight'] for r in b):.0f}-{max(r['fight'] for r in b):.0f}% | {statistics.median(r['time'] / r['T'] for r in b):.2f} | "
              f"{min(r['core'] for r in b):.0f}-{max(r['core'] for r in b):.0f} | {100 * sum(r['naive_loses'] for r in b) / len(b):.0f}% | "
              f"{statistics.median(r['lowest'] for r in b):.1f} | {statistics.median(r['units'] for r in b):.0f} |")

    # 3. The letters themselves.
    letters = collections.Counter("".join(r["patterns"] for r in data))
    total = sum(letters.values())
    print("\npattern share over all rows: " + ", ".join(f"{k} {100 * v / total:.0f}%" for k, v in letters.most_common()))
    late = band(40, 300)
    first = collections.Counter(r["patterns"][0] for r in late)
    print("first row from level 40: " + ", ".join(f"{k} {100 * v / len(late):.0f}%" for k, v in first.most_common()))
    hit = sum(1 for r in data if r["core"] < 10)
    print(f"levels where the greedy bot's median core is below 10: {hit} of {n}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))

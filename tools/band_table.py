"""band_table.py report.md [out.md]: the bake report as a table per band of ten levels, and the checks.

Per band: greedy's median time (in T) and core, the share it cancelled against the target, geodes opened of those shown,
how much slower podless is, and VARIETY_DESIGN 12.2 step 8's four shares:
  hungry  - levels where hungry is slower than greedy or loses (want 60% or more of the levels with geodes)
  unopened - levels with three or more geodes where greedy leaves one unopened (want half or more)
  pip     - levels where greedy loses a core pip in most runs (want 15 to 40%)
  deep    - levels where a gloom comes below the first gate row in most of greedy's runs (want 40% or more, from 21)
Checks (exit 1 when one fails): every level accepted; every cancelled share within 5 points of its target. Step 8's
shares are printed against their ranges; they are reported, not enforced here (VARIETY_PLAN V1 exit checks).
"""
import re
import statistics
import sys


def rows(path):
    out, cols = [], None
    for line in open(path, encoding="utf-8"):
        if line.startswith("| Level |"):
            cols = [c.strip() for c in line.strip().strip("|").split("|")]
            continue
        if not cols or not re.match(r"^\| \d+ \|", line):
            continue
        c = dict(zip(cols, [x.strip() for x in line.strip().strip("|").split("|")]))
        t = re.match(r"([\d.]+) \(([\d.]+)\)", c["Time (T)"])
        opened, shown = (float(v) for v in c["Opened"].split("/"))

        def bot(cell):
            m = re.match(r"(\d)/5 (\S+)", cell)
            return int(m.group(1)), (float(m.group(2)) if m.group(2) != "-" else float("inf"))

        pw, pt = bot(c["Podless"])
        hw, ht = bot(c["Hungry"])
        out.append({
            "level": int(c["Level"]), "accepted": c["Accepted"] == "yes", "h": float(c["h"]), "geodes": c["Geodes"],
            "ngeodes": 0 if c["Geodes"] == "-" else len(c["Geodes"].split()), "pa": float(c["Pa /s"]), "pn": float(c["Pn /s"]),
            "m": float(c["m"]), "target": float(c["Target"].rstrip("%")), "cancelled": float(c["Cancelled"].rstrip("%")),
            "time": float(t.group(1)), "T": float(t.group(2)), "core": float(c["Core"]), "opened": opened, "shown": shown,
            "pip": int(c["Pip lost"]), "deep": int(c["Deep gloom"]), "pw": pw, "pt": pt, "hw": hw, "ht": ht,
            "naive_loses": c["Naive"] == "loses", "seeds": int(c["Seeds"]),
            "plan": c.get("Plan", "Steady"), "lowz": float(c["Lowest gloom z"]),
        })
    return out


def share(rs, pred):
    rs = list(rs)
    return 100.0 * sum(1 for r in rs if pred(r)) / len(rs) if rs else float("nan")


def main(argv):
    data = rows(argv[0])
    lines = ["| Levels | Greedy time (T) | Core | Cancelled / target | Geodes opened | Podless slower | Hungry slower or loses | "
             "3+ geodes, one left | Pip lost | Gloom past row 1 | Seeds |",
             "|---|---|---|---|---|---|---|---|---|---|---|"]
    bands = {}
    for r in data:
        bands.setdefault((r["level"] - 1) // 10, []).append(r)
    misses = []
    for b in sorted(bands):
        rs = bands[b]
        lo, hi = rs[0]["level"], rs[-1]["level"]
        geo = [r for r in rs if r["ngeodes"] > 0]
        three = [r for r in rs if r["ngeodes"] >= 3]
        opened = sum(r["opened"] for r in geo)
        shown = sum(r["shown"] for r in geo)
        slower = [100.0 * (r["pt"] / r["time"] - 1.0) for r in geo if r["pt"] != float("inf")]
        hungry = share(geo, lambda r: r["hw"] < 3 or r["ht"] > r["time"])
        unopened = share(three, lambda r: r["opened"] < r["shown"])
        pip = share(rs, lambda r: r["pip"] >= 5)
        deep = share(rs, lambda r: r["deep"] >= 5)
        lines.append(
            f"| {lo}-{hi} | {statistics.median(r['time'] / r['T'] for r in rs):.2f} | {statistics.median(r['core'] for r in rs):.0f} | "
            f"{statistics.median(r['cancelled'] for r in rs):.0f}% / {statistics.median(r['target'] for r in rs):.0f}% | "
            f"{opened:.0f}/{shown:.0f} | {statistics.median(slower):.0f}% | {hungry:.0f}% | "
            f"{'-' if not three else f'{unopened:.0f}%'} | {pip:.0f}% | {deep:.0f}% | {sum(r['seeds'] for r in rs)} |"
            if geo and slower else
            f"| {lo}-{hi} | {statistics.median(r['time'] / r['T'] for r in rs):.2f} | {statistics.median(r['core'] for r in rs):.0f} | "
            f"{statistics.median(r['cancelled'] for r in rs):.0f}% / {statistics.median(r['target'] for r in rs):.0f}% | - | - | - | - | "
            f"{pip:.0f}% | {deep:.0f}% | {sum(r['seeds'] for r in rs)} |")
        if lo >= 21 and geo:
            if hungry < 60: misses.append(f"{lo}-{hi}: hungry slower or losing on {hungry:.0f}% (want 60%)")
            if three and unopened < 50: misses.append(f"{lo}-{hi}: one geode left on {unopened:.0f}% of 3+ geode levels (want 50%)")
            if not 15 <= pip <= 40: misses.append(f"{lo}-{hi}: greedy loses a pip on {pip:.0f}% (want 15-40%)")
            if deep < 40: misses.append(f"{lo}-{hi}: a gloom past the first row on {deep:.0f}% (want 40%)")
    # By the enemy's plan (VARIETY_PLAN V2: the point of plans is that the enemy is felt), levels 21 on.
    plans = {}
    for r in data:
        if r["level"] >= 21:
            plans.setdefault(r["plan"], []).append(r)
    if len(plans) > 1:
        lines += ["", "| Plan (levels 21 on) | Levels | Greedy core, lowest median | Pip lost | Lowest gloom z, median | Gloom past row 1 |",
                  "|---|---|---|---|---|---|"]
        for name in sorted(plans, key=lambda k: -len(plans[k])):
            rs = plans[name]
            lines.append(f"| {name} | {len(rs)} | {min(r['core'] for r in rs):.0f} | {share(rs, lambda r: r['pip'] >= 5):.0f}% | "
                         f"{statistics.median(r['lowz'] for r in rs):.1f} | {share(rs, lambda r: r['deep'] >= 5):.0f}% |")
    bad_share = [r for r in data if r["level"] > 5 and abs(r["cancelled"] - r["target"]) > 5.0]
    not_acc = [r["level"] for r in data if not r["accepted"]]
    lines += ["", f"Levels: {len(data)}, not accepted: {len(not_acc)} {not_acc[:30]}",
              f"Cancelled share more than 5 points from its target: {len(bad_share)} "
              + ", ".join(f"{r['level']} ({r['cancelled']:.0f}% vs {r['target']:.0f}%)" for r in bad_share[:30]),
              f"Step 8 outside its ranges (from level 21): {len(misses)}"] + [f"- {m}" for m in misses]
    text = "\n".join(lines)
    print(text)
    if len(argv) > 1:
        with open(argv[1], "w", encoding="utf-8") as fh:
            fh.write("# Difficulty by band of ten levels\n\nFrom the bake report (`tools/band_table.py`).\n\n" + text + "\n")
    return 0 if not bad_share and not not_acc else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))

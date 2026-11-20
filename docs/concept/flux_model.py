"""A toy model of GLOWTIDE's core contest, written while planning (2026-11-19).

It checks the arithmetic behind GAME_DESIGN 5.9 and 7.4 before any game code exists: a stream through
multiply gates against an enemy flow sized as a share of what the stream delivers. It is NOT the game:
power 1 only, no caps, no separation, no barriers, a fixed launcher. Its numbers are quoted in
GAME_DESIGN 5.9; the real simulation and the bake (P2) replace it.

Run:  python -I -B docs/concept/flux_model.py        (about three minutes)
"""
import numpy as np

DT, LEN, WISP_SPEED, GLOOM_SPEED, FIRE_RATE, HOMING = 1 / 60, 36.0, 8.0, 3.2, 12.0, 0.8
X30 = [(8.0, 0.0, 1.8, 2), (15.0, 0.0, 1.8, 3), (22.0, 0.0, 1.8, 5)]   # z, centre, half-width, factor
X15 = X30[1:]                                                         # the same level with the x2 missed
X6 = [(9.0, 0.0, 1.8, 2), (17.0, 0.0, 1.5, 3)]                        # level 3
EMITTERS = [-2.5, 0.0, 2.5]


def run(gates, rate_fn, seconds, launcher_x=0.0, spire=None, seed=1):
    """rate_fn(t) = glooms per second. Returns totals; stops early when `spire` power has been delivered."""
    rng = np.random.default_rng(seed)
    wx = np.zeros(0); wz = np.zeros(0); wv = np.zeros(0); wb = np.zeros(0, dtype=np.int64)
    gx = np.zeros(0); gz = np.zeros(0)
    shot, owed = 1.0, 0.0
    out = dict(produced=0, cancelled=0, delivered=0, core=0, lowest=LEN, time=seconds, won=False)
    for tick in range(int(seconds / DT)):
        t = tick * DT
        shot += FIRE_RATE * DT
        if shot >= 1:
            shot -= 1
            wx = np.append(wx, launcher_x + rng.uniform(-0.07, 0.07)); wz = np.append(wz, 1.0)
            wv = np.append(wv, 0.0); wb = np.append(wb, 0); out["produced"] += 1
        if t >= 1.5:
            owed += rate_fn(t) * DT
            n = int(owed); owed -= n
            if n:
                gx = np.append(gx, rng.choice(EMITTERS, n) + rng.uniform(-0.4, 0.4, n))
                gz = np.append(gz, np.full(n, LEN - 0.5))
        zprev = wz.copy()
        wz = wz + WISP_SPEED * DT
        wx = wx + wv * DT
        wv = wv * np.exp(-DT / 0.5)
        funnel = wz > LEN - 7
        wx = np.where(funnel, wx + np.clip(np.clip(wx, -2.5, 2.5) - wx, -3 * DT, 3 * DT), wx)
        wx = np.clip(wx, -4.74, 4.74)
        gx = gx + np.clip(launcher_x - gx, -HOMING * DT, HOMING * DT)
        gz = gz - GLOOM_SPEED * DT
        for gi, (z0, cx, hw, k) in enumerate(gates):
            idx = np.nonzero((zprev < z0) & (wz >= z0) & (np.abs(wx - cx) <= hw) & ((wb >> gi) & 1 == 0))[0]
            if len(idx):
                wb[idx] |= 1 << gi
                fan = 0.9 + 0.5 * k
                wv[idx] = fan * rng.uniform(-1, 1, len(idx))
                rep = np.repeat(idx, k - 1)
                wx = np.append(wx, wx[rep]); wz = np.append(wz, wz[rep] + rng.uniform(0, 0.15, len(rep)))
                zprev = np.append(zprev, wz[-len(rep):])          # clones are not tested again this tick
                wv = np.append(wv, fan * rng.uniform(-1, 1, len(rep))); wb = np.append(wb, wb[rep])
                out["produced"] += len(rep)
        if len(gx) and len(wx):                                   # clash, one for one
            d2 = (gx[:, None] - wx[None, :]) ** 2 + (gz[:, None] - wz[None, :]) ** 2
            j = np.argmin(d2, axis=1)
            near = np.nonzero(d2[np.arange(len(gx)), j] < 0.55 ** 2)[0]
            _, first = np.unique(j[near], return_index=True)
            gk = near[first]; wk = j[gk]
            out["cancelled"] += len(gk)
            keep_g = np.ones(len(gx), bool); keep_g[gk] = False
            keep_w = np.ones(len(wx), bool); keep_w[wk] = False
            gx, gz = gx[keep_g], gz[keep_g]
            wx, wz, wv, wb = wx[keep_w], wz[keep_w], wv[keep_w], wb[keep_w]
        home = wz >= LEN - 0.3
        out["delivered"] += int(home.sum())
        wx, wz, wv, wb = wx[~home], wz[~home], wv[~home], wb[~home]
        through = gz <= 0.8
        out["core"] += int(through.sum())
        gx, gz = gx[~through], gz[~through]
        if len(gz) and t > 6:
            out["lowest"] = min(out["lowest"], float(gz.min()))
        if spire and out["delivered"] >= spire:
            out["time"], out["won"] = t, True
            break
        if out["core"] >= 10 and spire:
            out["time"] = t
            break
    out["share"] = out["cancelled"] / max(1, out["produced"])
    return out


def line(name, r, seconds):
    print(f"{name:58s} delivered {r['delivered'] / seconds:6.1f}/s  cancelled {r['share'] * 100:5.1f}%  "
          f"core hits {r['core']:5d}  lowest gloom z {r['lowest']:5.1f}")


def enemy(fight, p0, T, scale=1.0, bursts=True):
    """The enemy's flow of GAME_DESIGN 5.9: base flow, two bursts of twice the flow, the ramp past 1.5 T."""
    def flow(t):
        burst = bursts and ((0.35 * T <= t < 0.35 * T + 3) or (0.70 * T <= t < 0.70 * T + 3))
        return fight * p0 * scale * (2.0 if burst else 1.0) * (1 + max(0.0, t - 1.5 * T) / 20)
    return flow


if __name__ == "__main__":
    T = 30.0
    FIGHTS = (0.13, 0.18, 0.30)          # levels 2, 10 and 30 on (GAME_DESIGN section 6)
    X20 = [(8.0, 0.0, 1.8, 2), (15.0, 0.0, 1.8, 2), (22.0, 0.0, 1.8, 5)]   # two thirds of the flood
    p0 = run(X30, lambda t: 0.0, T)["delivered"] / T
    print(f"x30 level, T = {T:.0f} s: P0 = {p0:.0f} power a second delivered with no enemy\n")

    print("1. A steady enemy flow of `fight` x P0: the cancelled share follows it; the line stays at the spire")
    for fight in FIGHTS:
        line(f"   fight {fight:.2f}", run(X30, enemy(fight, p0, T, bursts=False), T), T)

    print("\n2. With the two bursts: what rateScale brings the cancelled share to `fight`?")
    for fight in FIGHTS:
        for scale in (0.9, 1.0, 1.1):
            line(f"   fight {fight:.2f} rateScale {scale:.1f}", run(X30, enemy(fight, p0, T, scale), T), T)

    print("\n3. The cliff: an enemy flow held near or above what the flood can cancel")
    for mult in (0.9, 1.2, 1.5):
        line(f"   flow = {mult:.1f} x P0 for the whole level", run(X30, lambda t, m=mult: m * p0, T), T)

    print("\n4. Weaker players on the same level, spire sized for the full flood (0.85 x delivered)")
    for fight in FIGHTS:
        spire = int(0.85 * run(X30, enemy(fight, p0, T), T)["delivered"])
        for name, gates in (("full flood (x30)", X30), ("two thirds (x20)", X20), ("half (x15)", X15)):
            r = run(gates, enemy(fight, p0, T), 3.5 * T, spire=spire)
            print(f"   fight {fight:.2f}  {name:18s} {'wins ' if r['won'] else 'LOSES'} at {r['time']:5.1f} s "
                  f"= {r['time'] / T:.2f} T   core hits {r['core']:3d}   lowest gloom z {r['lowest']:4.1f}")

    print("\n5. Level 3 (x6, T = 18 s, no bursts), fight 0.14")
    p3 = run(X6, lambda t: 0.0, 18.0)["delivered"] / 18.0
    line(f"   P0 = {p3:.0f}/s", run(X6, enemy(0.14, p3, 18.0, bursts=False), 18.0), 18.0)

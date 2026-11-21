// The generator: a level from its number and seed. The calendar (calendar.cpp) chooses what the level is; a recipe from the
// catalogue (recipe_book.cpp, VARIETY_DESIGN 9.2) lays out the lane; this file adds the enemy, the geodes (9.3) and the
// twists for what the recipe left empty, and mirrors an asymmetric recipe half the time.
#include <cmath>

#include "core/rng.h"
#include "sim/level_build.h"

namespace gt::sim {

// The column a straight stream does best in, on the layout alone: the gates' factors where they stand, a slider
// counted for the share of its travel it spends over the column.
float bestLineX(const LevelDef& L) {
    float bestX = 0, best = -1;
    for (int s = 0; s <= 44; ++s) {
        const float x = -kLauncherRange + 0.2f * float(s);
        float m = columnMult(L, x, L.len);
        for (int i = 0; i < L.barrierCount; ++i)
            if (x >= L.barriers[i].x0 && x <= L.barriers[i].x1) m *= 0.8f;
        if (m > best + 1e-4f || (std::fabs(m - best) <= 1e-4f && std::fabs(x) < std::fabs(bestX))) {
            best = m;
            bestX = x;
        }
    }
    return bestX;
}

// A detour (VARIETY_DESIGN 9.3): off the best line by 2 m or more and outside the fan of the stream there, so only a
// stream aimed at it opens it; where the flood fills the top of the lane, it appears lower down, where the stream is
// narrower. On the preferred side when there is room.
void placeDetour(const LevelDef& L, Geode& g, float best, bool preferRight, float extra) {
    const float edge = 3.9f - g.amp, need = 2.0f + g.amp + extra;
    if (best + need > edge) preferRight = false;  // no room on that side: the other
    else if (best - need < -edge) preferRight = true;
    g.x = clampv(best + (preferRight ? need : -need), -edge, edge);
    // A colossus walks down the top of the lane and takes the stream there: its level's detours wait below it.
    const float top = L.kind == kColossus ? 0.55f * L.len : L.len - 3.0f, low = L.rowCount ? L.rowZ[0] + 2.0f : top;
    for (float z = top; z >= low; z -= 1.0f) {
        const float off = maxv(2.0f, fanSpread(L, best, z) + g.radius + 1.0f) + g.amp + extra;  // clear of the spill
        const bool rightOk = best + off <= edge, leftOk = best - off >= -edge;
        if (!rightOk && !leftOk) continue;
        g.x = rightOk && (!leftOk || preferRight) ? best + off : best - off;
        g.z0 = z < top ? z : 0.0f;
        break;
    }
}

// Whether half the rows or more have a gate that slides, hops or breathes, or a sliding wall just below them.
bool movingLane(const LevelDef& L) {
    int moving = 0;
    for (int row = 0; row < L.rowCount; ++row) {
        bool m = false;
        for (int g = 0; g < L.gateCount; ++g) m = m || (L.gates[g].row == row && (L.gates[g].slides || L.gates[g].breath > 0));
        for (int b = 0; b < L.barrierCount; ++b)
            m = m || (L.barriers[b].amp > 0 && L.barriers[b].z < L.rowZ[row] && L.barriers[b].z > L.rowZ[row] - 3.0f);
        moving += m ? 1 : 0;
    }
    return L.rowCount > 0 && moving * 2 >= L.rowCount;
}

// When a geode is on the lane, if nobody opens it: from its time (a time twist's, for one a twist releases; never, for
// SECOND WIND's, which comes with the core) until it drifts past the lantern, a vault after it leaves.
bool geodeWindow(const LevelDef& L, const Geode& G, float& t0, float& t1) {
    t0 = G.t;
    if (G.twist) {
        const TwistDef& x = L.twists[G.twist - 1];
        if (x.trigger != kTrigTime) return false;
        t0 = x.at * L.targetT;
    }
    const float z0 = G.z0 > 0 ? G.z0 : L.len - 3.0f;
    t1 = maxv(t0, G.leaveAt) + z0 / maxv(0.1f, G.speed);
    return true;
}

int geodesAtOnce(const LevelDef& L) {
    int most = 0;
    for (int i = 0; i < L.geodeCount; ++i) {
        float a0, a1;
        if (!geodeWindow(L, L.geodes[i], a0, a1)) continue;
        int n = 0;
        for (int j = 0; j < L.geodeCount; ++j) {
            float b0, b1;
            if (geodeWindow(L, L.geodes[j], b0, b1) && b0 <= a0 + 1e-4f && a0 < b1) ++n;
        }
        most = maxv(most, n);
    }
    return most;
}

// Geodes in a generated level (VARIETY_DESIGN 9.3): how many, their boons within the budget, when they come, and where:
// at least half of them away from the best line, the rest on it.
void placeGeodes(LevelDef& L, Rng& r) {
    const int index = L.index;
    if (index < 6) return;
    int n = r.rangei(index <= 20 ? 1 : 2, index <= 20 ? 2 : index <= 60 ? 3 : 4);
    if (signatureOf(L.world).geodes) n = minv(index <= 20 ? 2 : index <= 60 ? 3 : 4, n + 1);  // Gold Noon: one more
    if (L.boss) n = maxv(n, 2);
    const float budget = index <= 20 ? 2.0f : index <= 60 ? 2.5f : 3.0f;
    const float T = L.targetT, best = bestLineX(L);
    int lanterns = 1, quicken = 0, bright = 0, wards = 0, kinds = 0;
    // The level that introduces a boon offers it first (VARIETY_DESIGN 10.1): KINDLE 36, COMET 52, HUSH 63.
    const int intro = index == 7    ? kBoonQuicken
                      : index == 12 ? kBoonBright
                      : index == 36 ? kBoonKindle
                      : index == 52 ? kBoonComet
                      : index == 63 ? kBoonHush
                                    : -1;
    bool used[kBoonCount] = {};
    float worth = 1, t = (movingLane(L) ? r.range(0.08f, 0.12f) : r.range(0.08f, 0.25f)) * T;  // early where gates move
    for (int k = 0; k < n && L.geodeCount < kMaxGeodes; ++k) {
        if (k > 0) t += r.range(5.0f, 8.0f);
        if (L.boss && k == 1) t = minv(t, maxv(L.geodes[0].t + 5.0f, 0.4f * T));
        // The boons this geode may hold: from their first level, under their limits and the budget, at most three kinds a
        // level; the first geode up to level 60 lasts; a WARD at most once, never first, never at 75% of T or later.
        const float late = t > 0.75f * T;
        float wt[kBoonCount] = {};
        auto gainOf = [&](int b) {
            return b == kBoonKindle ? (used[kBoonKindle] ? 0.0f : kindleGain(L, best)) : boonGain(b, lanterns, quicken, bright);
        };
        auto allow = [&](int b, int from, float weight) {
            if (index < from || (!used[b] && kinds >= 3)) return;
            const float gain = gainOf(b);
            if (gain <= 0 || late || worth * (1.0f + gain) > budget + 1e-3f) return;
            wt[b] = weight;
        };
        allow(kBoonLantern, 6, k == 0 ? 4.0f : 3.0f);  // the first one is a detour: lean it toward a boon worth one
        allow(kBoonQuicken, 7, k == 0 ? 1.5f : 3.0f);
        allow(kBoonBright, 12, k == 0 ? 3.5f : 2.5f);
        allow(kBoonKindle, 36, 2.0f);
        if (index >= 23 && wards == 0 && (k > 0 || index > 60) && t < 0.85f * T && (used[kBoonWard] || kinds < 3)) wt[kBoonWard] = 1.5f;
        // The instants: COMET and HUSH at most once a level each; HUSH is rarer (it eases the contest: V2's notes).
        if (index >= 52 && !used[kBoonComet] && !late && (k > 0 || index > 60) && kinds < 3) wt[kBoonComet] = 1.2f;
        if (index >= 63 && !used[kBoonHush] && !late && kinds < 3) wt[kBoonHush] = 0.7f;
        // The introduction: the first geode; a COMET the second, which rides the best line (a comet fires from where the
        // lantern is when it opens: opened on a detour it climbs the detour's weak line).
        if (intro >= 0 && k == (intro == kBoonComet ? 1 : 0)) {
            for (float& x : wt) x = 0;
            wt[intro] = 1;
        }
        float total = 0;
        for (float x : wt) total += x;
        if (total <= 0) break;
        float pick = r.range(0, total);
        int boon = 0;
        while (boon < kBoonCount - 1 && pick >= wt[boon]) pick -= wt[boon++];
        if (wt[boon] <= 0) break;
        if (!used[boon]) used[boon] = true, ++kinds;
        worth *= 1.0f + gainOf(boon);
        if (boon == kBoonLantern) ++lanterns;
        if (boon == kBoonQuicken) ++quicken;
        if (boon == kBoonBright) ++bright;
        if (boon == kBoonWard) ++wards;

        Geode g;
        g.t = t;
        g.boon = u8(boon);
        g.speed = r.range(1.2f, 2.0f);
        const bool detour = k % 2 == 0;
        if (detour && index >= 40 && r.f01() < 0.25f) {  // a slide, on detours only: a moving geode is harder to hit
            g.amp = r.range(0.5f, 0.9f);
            g.period = r.range(3.0f, 5.0f);
            g.phase = r.f01();
        }
        const float edge = 3.9f - g.amp;
        if (detour) {
            const bool preferRight = r.f01() < 0.5f;
            placeDetour(L, g, best, preferRight, r.range(0.2f, 0.8f));
        } else {  // on the line
            g.x = clampv(best + r.range(-0.6f, 0.6f), -edge, edge);
        }
        // Plates on detours only, and not where the gates move: the stream sweeps there, and a plate made the boon cost
        // more than it paid. There a detour comes low on the lane, where the stream is still one line, and costs less.
        const bool moving = movingLane(L);
        if (detour && moving) {
            g.z0 = L.rowCount ? L.rowZ[0] + 2.0f : 0.0f;
            g.speed = 1.0f;
            g.hpSeconds = r.range(1.2f, 1.6f);
        } else if (detour && index >= 11 && boon != kBoonWard && r.f01() < (index <= 20 ? 0.3f : 0.4f)) {
            g.hpSeconds = r.range(1.0f, 1.5f);
            g.shieldSeconds = r.range(3.0f, 4.0f);
        } else {
            g.hpSeconds = boon == kBoonWard ? r.range(1.5f, 2.0f) : r.range(2.0f, 3.0f);
        }
        L.geodes[L.geodeCount++] = g;
    }
}

// A festival's rain (VARIETY_DESIGN 6): 10 to 14 small geodes falling over its 20 s, boons from the pool and under their
// limits (no WARD or HUSH: there is no enemy), COMET for the rest.
void placeFestivalGeodes(LevelDef& L, Rng& r) {
    const int n = r.rangei(10, 14);
    int lanterns = 1, quicken = 0, bright = 0, kindle = 0;
    // Where most gates slide or breathe, a still stream up the middle is swept across the rain and opened as many geodes
    // as greedy (V5's bake): there the rain falls further out and is tougher.
    int sweeping = 0;
    for (int i = 0; i < L.gateCount; ++i) sweeping += L.gates[i].slides == kGateSlide || L.gates[i].breath > 0 ? 1 : 0;
    const bool swept = L.gateCount > 0 && 2 * sweeping >= L.gateCount;
    for (int k = 0; k < n && L.geodeCount < kMaxGeodes; ++k) {
        Geode g;
        g.t = 1.0f + 15.0f * float(k) / float(n) + r.range(0.0f, 0.6f);
        g.x = (r.f01() < 0.5f ? -1.0f : 1.0f) * r.range(swept ? 2.2f : 1.4f, 3.6f);  // off the middle: a still stream catches few
        g.speed = r.range(1.8f, 2.6f);
        g.radius = 0.6f;
        g.hpSeconds = r.range(0.5f, 0.9f) * (swept ? 1.6f : 1.0f);
        int b = kBoonComet;
        const float roll = r.f01();
        if (roll < 0.25f && lanterns < int(tune.lanternMax)) b = kBoonLantern, ++lanterns;
        else if (roll < 0.5f && quicken < int(tune.quickenMax)) b = kBoonQuicken, ++quicken;
        else if (roll < 0.7f && bright < int(tune.brightMax)) b = kBoonBright, ++bright;
        else if (roll < 0.8f && kindle < 1 && L.index >= 36) b = kBoonKindle, ++kindle;
        g.boon = u8(b);
        L.geodes[L.geodeCount++] = g;
    }
}

// A dormant wall across the whole lane, 6 m below the spire, raised by WARD WALL; its health is set with the spire's.
static u8 addWardWall(LevelDef& L) {
    if (L.barrierCount >= kMaxBarriers) return 0;
    Barrier b;
    b.x0 = -kHW + 0.1f;
    b.x1 = kHW - 0.1f;
    b.z = L.len - 6.0f;
    b.health = 10;
    b.dormant = 1;
    L.barriers[L.barrierCount] = b;
    return u8(L.barrierCount++);
}

// The boss's script (VARIETY_DESIGN 8): two twists at 66% and 33% of its health.
void addBossScript(LevelDef& L) {
    switch (L.bossType) {
        case kBossWarden:
            addTwist(L, kTwistWardWall, kTrigGoal, 0.66f, 0, addWardWall(L));
            addTwist(L, kTwistBruteGuard, kTrigGoal, 0.33f, 2);
            break;
        case kBossTempest:
            addTwist(L, kTwistStampede, kTrigGoal, 0.66f, 0, 0);
            addTwist(L, kTwistEclipse, kTrigGoal, 0.33f);
            break;
        case kBossHydra:
            addTwist(L, kTwistCub, kTrigGoal, 0.66f, 0, 1);
            addTwist(L, kTwistCub, kTrigGoal, 0.33f, 0, 2);
            break;
        default: {  // THE OLD SPIRE: today's boss, a surge with two brutes at each threshold
            TwistDef& a = addTwist(L, kTwistSurge, kTrigGoal, 0.66f, 2);
            a.dur = 4.0f;
            TwistDef& b = addTwist(L, kTwistBruteGuard, kTrigGoal, 0.33f, 2);
            b.dur = 4.0f;
            break;
        }
    }
}

// A geode a twist releases (GEODE RAIN's runners, SECOND WIND's WARD).
void addTwistGeode(LevelDef& L, int twist, float x, float z0, float speed, float radius, float hpSeconds, u8 boon) {
    if (L.geodeCount >= kMaxGeodes) return;
    Geode g;
    g.x = x;
    g.z0 = z0;
    g.speed = speed;
    g.radius = radius;
    g.hpSeconds = hpSeconds;
    g.boon = boon;
    g.twist = u8(twist + 1);
    L.geodes[L.geodeCount++] = g;
}

// Twists on a generated level (VARIETY_DESIGN 7, 10.2): SECOND WIND from level 23 wherever there is an enemy (the one
// comeback: it fires only when the core falls to 3); from level 43, on levels that are not bosses or festivals, by the
// level's hardness: none below 0.4, one, two from 0.8; drawn by the seed among those that have come, the introduced one
// first on its level.
void addTwists(LevelDef& L, Rng& r) {
    if (!L.hasEnemy()) return;
    int count = L.index < 43 || L.kind == kBoss || L.kind == kFestival ? 0 : L.hardness < 0.4f ? 0 : L.hardness < 0.8f ? 1 : 2;
    if (signatureOf(L.world).twists && L.index >= 43 && L.kind != kBoss && L.kind != kFestival)  // Storm Indigo: one more, from h 0.2
        count = L.hardness < 0.2f ? 0 : L.hardness < 0.65f ? 1 : 2;
    if (L.twistCount > 0) count = 0;  // a set piece chose its own
    if (introTwist(L.index) >= 0 && L.kind != kBoss && L.kind != kFestival) count = maxv(count, 1);
    for (int n = 0; n < count && L.twistCount < kMaxTwists - 1; ++n) {
        u8 pool[6];
        int m = 0;
        pool[m++] = kTwistSurge;
        pool[m++] = kTwistBruteGuard;
        pool[m++] = kTwistGeodeRain;
        if (L.index >= 48) pool[m++] = kTwistEclipse;
        pool[m++] = kTwistStampede;
        if (L.index >= 66) pool[m++] = kTwistGoldenGate;
        u8 type = pool[r.rangei(0, m - 1)];
        if (n == 0 && introTwist(L.index) >= 0) type = u8(introTwist(L.index));
        for (int k = 0; k < L.twistCount && n > 0; ++k)  // two twists of one kind would be one twist twice
            if (L.twists[k].type == type) type = type == kTwistSurge ? kTwistBruteGuard : kTwistSurge;
        const int k = L.twistCount;
        TwistDef& x = addTwist(L, type, kTrigTime, n == 0 ? r.range(0.3f, 0.5f) : r.range(0.55f, 0.75f), type == kTwistBruteGuard ? 2 : 0);
        if (type == kTwistStampede) x.param = u8(r.rangei(0, L.emitterCount - 1));
        if (type == kTwistGoldenGate) {  // the strongest multiply gate of the middle row
            const int row = L.rowCount / 2;
            int best = -1;
            for (int g = 0; g < L.gateCount; ++g)
                if (L.gates[g].row == row && L.gates[g].kind == kGateMul && (best < 0 || L.gates[g].k > L.gates[best].k)) best = g;
            x.param = u8(maxv(0, best));
        }
        if (type == kTwistGeodeRain) {  // three runner geodes at once, across the lane
            static const u8 boons[3] = {kBoonQuicken, kBoonBright, kBoonLantern};
            for (int j = 0; j < 3; ++j)
                addTwistGeode(L, k, -2.6f + 2.6f * float(j) + r.range(-0.4f, 0.4f), 0, 3.0f, 0.6f, r.range(0.6f, 0.9f), boons[(j + r.rangei(0, 2)) % 3]);
            // Never more than four geodes on the lane together (VARIETY_DESIGN 9.3): the rain moves to a time with room, or
            // (unless it is the introduction's, which the bake's next seed tries again) it becomes a SURGE.
            for (float at = 0.3f; geodesAtOnce(L) > 4 && at <= 0.751f; at += 0.05f) L.twists[k].at = at;
            if (geodesAtOnce(L) > 4 && introTwist(L.index) != kTwistGeodeRain) {
                const float at = L.twists[k].at;
                L.geodeCount -= 3;
                --L.twistCount;
                addTwist(L, kTwistSurge, kTrigTime, at);
            }
            // The introduction keeps its rain: geodes placed before it go instead, a pair or a caravan whole, never the first,
            // and one on the best line before a detour (VARIETY_DESIGN 9.3's place rule holds for what stays).
            const float best = bestLineX(L);
            while (geodesAtOnce(L) > 4) {
                const int placed = L.geodeCount - 3;  // the rain is the last three
                int groupStart[kMaxGeodes], groups = 0;
                for (int g = 0; g < placed; ++g)
                    if (g == 0 || !(L.geodes[g].t == L.geodes[g - 1].t || (L.geodes[g].group && L.geodes[g].group == L.geodes[g - 1].group)))
                        groupStart[groups++] = g;
                if (groups <= 1) break;
                int pick = groups - 1;
                for (int k = groups - 1; k >= 1; --k) {
                    const int end = k + 1 < groups ? groupStart[k + 1] : placed;
                    bool detour = false;
                    for (int g = groupStart[k]; g < end; ++g) detour = detour || std::fabs(L.geodes[g].x - best) + 0.5f * L.geodes[g].amp >= 2.0f;
                    if (!detour) {
                        pick = k;
                        break;
                    }
                }
                const int a = groupStart[pick], b = pick + 1 < groups ? groupStart[pick + 1] : placed;
                for (int g = b; g < L.geodeCount; ++g) L.geodes[g - (b - a)] = L.geodes[g];
                L.geodeCount -= b - a;
            }
        }
    }
    if (L.index >= 23 && L.twistCount < kMaxTwists) {
        const int k = L.twistCount;
        addTwist(L, kTwistSecondWind, kTrigCore, 3.0f);
        addTwistGeode(L, k, 0.0f, 0.5f * L.len, 1.0f, 0.9f, 0.8f, kBoonWard);
    }
}

// Mirrors a layout left to right (an asymmetric recipe's twin, and the replays after the last level: VARIETY_DESIGN 16):
// every x, every slide's phase by half a period (easedWave(p + 0.5) = -easedWave(p)), every door's side.
void mirrorLayout(LevelDef& L) {
    for (int i = 0; i < L.gateCount; ++i) {
        Gate& G = L.gates[i];
        G.cx = -G.cx;
        if (G.slides == kGateSlide) G.phase += 0.5f;
        if (G.slides == kGateHop) G.phase += 0.5f;
    }
    for (int i = 0; i < L.barrierCount; ++i) {
        Barrier& B = L.barriers[i];
        const float x0 = B.x0;
        B.x0 = -B.x1;
        B.x1 = -x0;
        if (B.amp > 0) B.phase += 0.5f;
    }
    for (int i = 0; i < L.bumperCount; ++i) L.bumpers[i].x = -L.bumpers[i].x;
    for (int i = 0; i < L.snufferCount; ++i) {
        Snuffer& S = L.snuffers[i];
        const float xa = S.xa;
        S.xa = -S.xb;
        S.xb = -xa;
        S.phase += 0.5f;
    }
    for (int i = 0; i < L.emitterCount; ++i) {
        Emitter& E = L.emitters[i];
        E.x = -E.x;
        E.side = i8(-E.side);
        if (E.amp > 0) E.phase += 0.5f;
    }
    for (int i = 0; i < L.geodeCount; ++i) {
        Geode& G = L.geodes[i];
        G.x = -G.x;
        if (G.amp > 0) G.phase += 0.5f;
    }
    if (L.kind == kTwins) L.twinShare = 1.0f - L.twinShare;
}

LevelDef generate(int index, u64 seed) { return generateWith(index, seed, -1); }

// A level from a recipe (-1: the calendar's choice, drawn by the seed among the recipes allowed there).
LevelDef generateWith(int index, u64 seed, int forced) {
    LevelDef L;
    L.index = index;
    L.seed = seed;
    L.world = worldFor(index);
    L.hardness = hardnessFor(index);
    L.fight = fightFor(index) + (L.hardness - 0.5f) * 0.08f;  // a world's rhythm moves the enemy's share (VARIETY_DESIGN 10.2)
    L.kind = u8(kindFor(index));
    L.boss = L.kind == kBoss;
    if (L.boss) L.bossType = u8((index / 10 - 1) % kBossCount);
    L.lengthClass = u8(lengthClassFor(index));
    L.targetT = targetTime(index);
    const bool festival = L.kind == kFestival;
    Rng r(seed, 7);

    // The recipe: the introduction's, the set piece's, or one the calendar allows here, drawn by the seed.
    int recipe = forced >= 0 ? forced : setPieceAt(index) >= 0 ? recipeCount() + setPieceAt(index) : introRecipe(index);
    if (recipe < 0 || recipe >= recipeCount() + setPieceCount()) recipe = pickRecipe(index, L.kind, r);
    L.recipe = u8(recipe);
    const RecipeRow& row = recipe < recipeCount() ? kRecipes[recipe] : kSetPieces[recipe - recipeCount()];
    Band band = bandFor(index);
    if (festival) {  // rich gates: no traps, patrols or crossing divides, one factor more
        band.strongest = minv(5, band.strongest + 1);
        band.patternWeight[kPatTrap] = band.patternWeight[kPatPatrol] = band.patternWeight[kPatCrossing] = 0;
    }
    RecipeCtx ctx;
    ctx.index = index;
    ctx.strongest = band.strongest;
    ctx.lengthClass = L.lengthClass;
    ctx.kind = L.kind;
    ctx.patternWeight = band.patternWeight;
    L.plan = 255;  // a recipe with an idea about the enemy sets it
    row.build(L, r, ctx);
    if (recipe >= recipeCount()) {
        // A set piece is laid out by hand; its seed gives the bake versions to balance: the enemy's share up to 15% lower
        // (and the mirror, below).
        L.fight *= r.range(0.85f, 1.0f);
    }
    if (row.mirror && r.f01() < 0.5f) mirrorLayout(L);
    L.targetT = targetTime(index, L.lengthClass);  // a recipe may have lengthened the lane

    // The enemy (GAME_DESIGN 5.9, 7.3 step 3) and its plan (VARIETY_DESIGN 5.1); a boss's plan is its own (8).
    const int introPlan = introPlanFor(index);
    const u8 recipePlan = L.plan;
    L.plan = u8(introPlan >= 0 ? introPlan : planFor(index, r));
    if (recipePlan != 255) L.plan = recipePlan;
    if (L.boss) {
        static const u8 bossPlan[kBossCount] = {kPlanSteady, kPlanSteady, kPlanRunnerPacks, kPlanPulse};
        L.plan = bossPlan[L.bossType];
    }
    const bool ownEmitters = L.emitterCount > 0;
    if (L.kind == kTwins && (L.plan == kPlanPincer || L.plan == kPlanFlank)) L.plan = kPlanPulse;  // each spire has its emitter
    if (ownEmitters && L.plan == kPlanFlank && recipePlan != kPlanFlank) L.plan = kPlanSteady;    // the recipe placed the enemy
    if (ownEmitters && L.plan == kPlanPincer && L.emitterCount != 2) L.plan = kPlanSteady;
    if (festival) L.plan = kPlanSteady, L.emitterCount = 0;
    const int emitters = index <= 9 ? 1 : index <= 29 ? 2 : 3;
    if (festival || ownEmitters) {
        // No enemy (a festival), or the recipe's own emitters.
    } else if (L.kind == kTwins) {  // an emitter at each spire's foot; it stops when its spire falls
        addEmitter(L, -tune.twinX);
        addEmitter(L, tune.twinX);
        L.emitters[1].group = 1;
    } else if (L.plan == kPlanPincer) {  // two emitters far apart, one at a time
        const float x = r.range(2.8f, 3.4f);
        addEmitter(L, -x);
        addEmitter(L, x);
    } else if (emitters == 1) {
        addEmitter(L, r.range(-2.0f, 2.0f));
    } else if (emitters == 2) {
        const float x = r.range(1.5f, 2.5f);
        addEmitter(L, -x);
        addEmitter(L, x);
    } else {
        addEmitter(L, -2.5f);
        addEmitter(L, 0);
        addEmitter(L, 2.5f);
    }
    if (L.kind == kTwins) L.twinShare = r.f01() < 0.5f ? 0.5f : (r.f01() < 0.5f ? 0.4f : 0.6f);
    if (!ownEmitters && index >= 20 && L.plan != kPlanPincer && L.emitterCount > 0 && r.f01() < 0.4f) {
        Emitter& e = L.emitters[r.rangei(0, L.emitterCount - 1)];
        e.amp = r.range(1.0f, 1.5f);
        e.period = r.range(4, 7);
        e.x = clampv(e.x, -2.5f + e.amp, 2.5f - e.amp);
    }
    if (!ownEmitters && L.plan == kPlanFlank) {
        // A third of the flow through one or two side doors, between z 14 and len - 8, clear of the rows by 1.5 m.
        const int foot = L.emitterCount, doors = r.f01() < 0.5f ? 1 : 2;
        for (int e = 0; e < foot; ++e) L.emitters[e].share = (2.0f / 3.0f) / float(foot);
        const int side = r.f01() < 0.5f ? -1 : 1;
        for (int d = 0; d < doors; ++d) {
            float dz = 0;
            for (int tries = 0; tries < 16 && dz == 0; ++tries) {
                const float c = r.range(14.0f, maxv(14.5f, L.len - 8.0f));
                bool clear = true;
                for (int i = 0; i < L.rowCount; ++i) clear = clear && std::fabs(c - L.rowZ[i]) >= 1.5f;
                if (clear) dz = c;
            }
            if (dz == 0) dz = maxv(14.0f, L.len - 8.0f);
            addDoor(L, d == 0 ? side : -side, dz, (1.0f / 3.0f) / float(doors));
        }
    }
    if (L.boss) addBossScript(L);
    applyEnemyDefaults(L);
    if (festival) placeFestivalGeodes(L, r);
    else if (L.geodeCount == 0) placeGeodes(L, r);
    addTwists(L, r);
    return L;
}

}  // namespace gt::sim

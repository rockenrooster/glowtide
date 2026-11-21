// A level's lints: GAME_DESIGN 7.3's rules for a layout and VARIETY_DESIGN 4, 6, 7 and 9.3's. The generator's tests run them
// on every recipe and the calendar's levels; the bake's first pass takes only a layout that passes them.
#include <cmath>
#include <cstdio>
#include <initializer_list>

#include "sim/level_build.h"

namespace gt::sim {

bool lintLevel(const LevelDef& L, int index, char* why, size_t whyLen) {
    bool ok = L.rowCount >= 2 && L.len >= 30 && L.len <= 44 && L.gateCount <= kMaxGates && (L.emitterCount >= 1 || L.kind == kFestival);
    if (!ok) std::snprintf(why, whyLen, "rows %d len %.1f gates %d", L.rowCount, double(L.len), L.gateCount);
    if (ok && (L.rowZ[0] < 8 || L.rowZ[0] > 10)) ok = false, std::snprintf(why, whyLen, "first row at %.1f", double(L.rowZ[0]));
    for (int r = 1; ok && r < L.rowCount; ++r) {
        const float gap = L.rowZ[r] - L.rowZ[r - 1];
        if (gap < 6 - 1e-3f || gap > 8 + 1e-3f) ok = false, std::snprintf(why, whyLen, "rows %.1f apart", double(gap));
    }
    if (ok && L.rowZ[L.rowCount - 1] > L.len - 10 + 1e-3f) ok = false, std::snprintf(why, whyLen, "last row too near the spire");
    for (int r = 0; ok && r < L.rowCount; ++r) {
        bool mul = false;
        for (int g = 0; g < L.gateCount; ++g) mul = mul || (L.gates[g].row == r && L.gates[g].kind == kGateMul);
        if (!mul) ok = false, std::snprintf(why, whyLen, "row %d has no multiply gate", r);
    }
    // Gates of a row never overlap (crossing sliders cross on purpose); openings stay inside the lane; a hopper or a
    // breathing gate is never also a slider (VARIETY_DESIGN 4).
    for (int a = 0; ok && a < L.gateCount; ++a) {
        const Gate& A = L.gates[a];
        const float reach = A.slides ? A.amp : A.breath > 0 ? 0.2f * A.hw : 0.0f;
        if (std::fabs(A.cx) + reach + A.hw > kHW + 1e-3f) ok = false, std::snprintf(why, whyLen, "gate %d leaves the lane", a);
        if (A.hw < 1.2f - 1e-3f || A.hw > 1.8f + 1e-3f) ok = false, std::snprintf(why, whyLen, "gate %d half-width %.2f", a, double(A.hw));
        if (A.breath > 0 && A.slides) ok = false, std::snprintf(why, whyLen, "gate %d breathes and moves", a);
        if (A.slides == kGateHop && (A.period < 1.5f - 1e-3f || A.period > 2.5f + 1e-3f))
            ok = false, std::snprintf(why, whyLen, "hopper %d stays %.2f s", a, double(A.period));
        for (int b = a + 1; ok && b < L.gateCount; ++b) {
            const Gate& B = L.gates[b];
            if (std::fabs(A.z - B.z) > 0.01f || A.slides || B.slides) continue;
            const float wa = A.hw * (A.breath > 0 ? 1.2f : 1.0f), wb = B.hw * (B.breath > 0 ? 1.2f : 1.0f);
            if (std::fabs(A.cx - B.cx) < wa + wb - 1e-3f) ok = false, std::snprintf(why, whyLen, "gates %d and %d overlap", a, b);
        }
    }
    // Sliding walls stay inside the lane; side doors stand on a rail between z 14 and len - 8; shares add up to 1.
    for (int a = 0; ok && a < L.barrierCount; ++a) {
        const Barrier& B = L.barriers[a];
        if (B.x0 - B.amp < -kHW - 1e-3f || B.x1 + B.amp > kHW + 1e-3f) ok = false, std::snprintf(why, whyLen, "wall %d leaves the lane", a);
    }
    float shares = 0;
    for (int e = 0; ok && e < L.emitterCount; ++e) {
        const Emitter& E = L.emitters[e];
        shares += E.share;
        if (E.side && (E.z < 14.0f - 1e-3f || E.z > maxv(14.0f, L.len - 8.0f) + 1e-3f))
            ok = false, std::snprintf(why, whyLen, "side door at z %.1f", double(E.z));
    }
    if (ok && shares > 0 && std::fabs(shares - 1.0f) > 1e-3f) ok = false, std::snprintf(why, whyLen, "shares add to %.3f", double(shares));
    // Twists: a boss has its two, a level from 43 one more, SECOND WIND from 23; no brutes but through a plan or a twist.
    int extra = 0, winds = 0;
    for (int k = 0; k < L.twistCount; ++k) {
        if (L.twists[k].type == kTwistSecondWind) ++winds;
        else ++extra;
    }
    if (ok && (L.bruteEvery > 0 || extra > (L.boss || (index >= 43 && L.kind != kFestival) ? 2 : 0) || winds != (index >= 23 && L.hasEnemy() ? 1 : 0)))
        ok = false, std::snprintf(why, whyLen, "%d twists and %d second winds", extra, winds);
    if (ok && L.kind != kindFor(index)) ok = false, std::snprintf(why, whyLen, "kind %s", kindName(L.kind));
    if (ok && L.kind == kFestival && (L.hasEnemy() || L.geodeCount < 10)) ok = false, std::snprintf(why, whyLen, "a festival with %d geodes", L.geodeCount);
    if (ok && index < planFrom(L.plan)) ok = false, std::snprintf(why, whyLen, "plan %s before its level", planName(L.plan));
    // Gaps: a bumper to the walls, to other bumpers and to gate posts at least 1.2 m.
    for (int a = 0; ok && a < L.bumperCount; ++a) {
        const Bumper& A = L.bumpers[a];
        if (kHW - std::fabs(A.x) - A.r < 1.2f - 1e-3f) ok = false, std::snprintf(why, whyLen, "bumper %d near a wall", a);
        for (int b = a + 1; ok && b < L.bumperCount; ++b) {
            const Bumper& B = L.bumpers[b];
            const float d = std::sqrt((A.x - B.x) * (A.x - B.x) + (A.z - B.z) * (A.z - B.z)) - A.r - B.r;
            if (d < 1.2f - 1e-3f) ok = false, std::snprintf(why, whyLen, "bumpers %d and %d %.2f m apart", a, b, double(d));
        }
        for (int g = 0; ok && g < L.gateCount; ++g) {
            const Gate& G = L.gates[g];
            if (G.slides) continue;
            for (float s2 : {-1.0f, 1.0f}) {
                const float px = G.cx + s2 * G.hw;
                const float d = std::sqrt((A.x - px) * (A.x - px) + (A.z - G.z) * (A.z - G.z)) - A.r - 0.18f;
                if (d < 1.2f - 1e-3f) ok = false, std::snprintf(why, whyLen, "bumper %d %.2f m from a post", a, double(d));
            }
        }
    }
    // Geodes (VARIETY_DESIGN 9.3): how many, when, how far apart, at most three kinds, the first lasting up to 60, the
    // budget, and inside the lane where a stream can reach them.
    // Geodes placed by VARIETY_DESIGN 9.3 (not a festival's rain, not the ones a twist releases).
    int placed = 0;  // a choice pair or a caravan (geodes that come together) counts once
    for (int g = 0; g < L.geodeCount; ++g)
        placed += L.geodes[g].twist == 0 && !(g > 0 && L.geodes[g - 1].twist == 0 &&
                                              (L.geodes[g].t == L.geodes[g - 1].t || (L.geodes[g].group && L.geodes[g].group == L.geodes[g - 1].group)))
                      ? 1 : 0;
    const int maxGeodes = index <= 20 ? 2 : index <= 60 ? 3 : 4;
    if (L.kind == kFestival) placed = 1;  // its rain is checked above
    if (ok && (placed > maxv(maxGeodes, L.boss ? 2 : 0) || (index >= 6 && placed == 0)))
        ok = false, std::snprintf(why, whyLen, "%d geodes", placed);
    float worth = 1;
    int lanterns = 1, quicken = 0, bright = 0, kinds = 0;
    bool seen[kBoonCount] = {};
    for (int g = 0; ok && L.kind != kFestival && g < L.geodeCount; ++g) {
        const Geode& G = L.geodes[g];
        if (G.twist) continue;
        const bool together = g > 0 && (G.t == L.geodes[g - 1].t || (G.group && G.group == L.geodes[g - 1].group));
        const bool lasting = G.boon != kBoonWard && G.boon != kBoonComet && G.boon != kBoonHush;
        if (g == 0 && (G.t < 0.08f * L.targetT - 1e-3f || G.t > 0.25f * L.targetT + 1e-3f))
            ok = false, std::snprintf(why, whyLen, "first geode at %.1f s of %.0f", double(G.t), double(L.targetT));
        if (g > 0 && !together && G.t < L.geodes[g - 1].t + 5.0f - 1e-3f) ok = false, std::snprintf(why, whyLen, "geodes under 5 s apart");
        if (G.boon != kBoonWard && G.t > 0.75f * L.targetT + 1e-3f) ok = false, std::snprintf(why, whyLen, "a boon after 75%% of T");
        if (g == 0 && index <= 60 && !lasting) ok = false, std::snprintf(why, whyLen, "the first geode is an instant");
        const int from[kBoonCount] = {6, 7, 12, 23, 36, 52, 63};
        if (index < from[G.boon]) ok = false, std::snprintf(why, whyLen, "%s before its level", boonName(G.boon));
        if (std::fabs(G.x) + G.amp > 3.9f + 1e-3f) ok = false, std::snprintf(why, whyLen, "geode %d outside the reachable lane", g);
        if (G.hp <= 0 || (G.shieldSeconds > 0 && G.shield <= 0)) ok = false, std::snprintf(why, whyLen, "geode %d has no health", g);
        if (!seen[G.boon]) seen[G.boon] = true, ++kinds;
        if (together && G.group) continue;  // one of a choice pair is taken, not both
        worth *= 1.0f + boonGain(G.boon, lanterns, quicken, bright);
        lanterns += G.boon == kBoonLantern;
        quicken += G.boon == kBoonQuicken;
        bright += G.boon == kBoonBright;
    }
    if (ok && kinds > 3) ok = false, std::snprintf(why, whyLen, "%d kinds of boon", kinds);
    // Place (9.3): at least half of the geodes placed (rounded up; a pair or a caravan counts once, and is a detour when
    // one of it is) stand 2 m or more off the best straight line (a sliding geode by half its swing).
    if (ok && L.kind != kFestival && placed > 0) {
        const float best = bestLineX(L);
        int detours = 0;
        for (int g = 0; g < L.geodeCount; ++g) {
            const Geode& G = L.geodes[g];
            if (G.twist) continue;
            const bool together = g > 0 && L.geodes[g - 1].twist == 0 && (G.t == L.geodes[g - 1].t || (G.group && G.group == L.geodes[g - 1].group));
            const bool off = std::fabs(G.x - best) + 0.5f * G.amp >= 2.0f - 1e-3f;
            bool groupOff = off;
            for (int h = g + 1; h < L.geodeCount && L.geodes[h].twist == 0 &&
                                (L.geodes[h].t == G.t || (L.geodes[h].group && L.geodes[h].group == G.group));
                 ++h)
                groupOff = groupOff || std::fabs(L.geodes[h].x - best) + 0.5f * L.geodes[h].amp >= 2.0f - 1e-3f;
            if (!together && groupOff) ++detours;
        }
        if (detours * 2 < placed) ok = false, std::snprintf(why, whyLen, "%d of %d geodes are detours (best line %.1f)", detours, placed, double(best));
    }
    // At once (9.3): outside a festival, never more than four geodes on the lane together, a twist's rain included.
    if (ok && L.kind != kFestival && geodesAtOnce(L) > 4) ok = false, std::snprintf(why, whyLen, "%d geodes on the lane at once", geodesAtOnce(L));
    if (ok && worth > (index <= 20 ? 2.0f : index <= 60 ? 2.5f : 3.0f) + 1e-3f)
        ok = false, std::snprintf(why, whyLen, "boons worth x%.2f over the budget", double(worth));
    return ok;
}

}  // namespace gt::sim

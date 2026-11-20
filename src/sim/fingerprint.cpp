#include "sim/fingerprint.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

namespace gt::sim {

namespace {

struct Map {
    Fingerprint& fp;
    float z0, bandLen;
    int band(float z) const { return clampv(int(std::floor((z - z0) / bandLen)), 0, kFpBands - 1); }
    static int col(float x) { return clampv(int(std::floor((x + kHW) / 1.25f)), 0, kFpCols - 1); }
    // Covers x0..x1 across and za..zb along with `code`; a stronger code wins a cell, an equal one adds its motion.
    void cover(float x0, float x1, float za, float zb, u8 code, bool moves) {
        for (int b = band(za); b <= band(zb); ++b)
            for (int c = col(x0); c <= col(x1); ++c) {
                u8& cell = fp.cells[b * kFpCols + c];
                const u8 have = cell & u8(~kFpMoves);
                if (code > have) cell = u8(code | (moves ? kFpMoves : 0));
                else if (code == have && moves) cell |= kFpMoves;
            }
    }
};

}  // namespace

Fingerprint fingerprintOf(const LevelDef& L) {
    Fingerprint fp;
    fp.kind = L.kind;
    fp.recipe = L.recipe;
    fp.plan = L.plan;
    fp.lengthClass = L.lengthClass;
    Map m{fp, 4.0f, maxv(1.0f, L.len - 8.0f) / float(kFpBands)};
    for (int i = 0; i < L.gateCount; ++i) {
        const Gate& G = L.gates[i];
        const float reach = G.slides ? G.amp : 0.0f;
        const u8 code = G.kind == kGateMul ? u8(kFpMul + clampv(int(G.k), 2, 6)) : u8(kFpDiv);
        m.cover(G.cx - reach - G.hw, G.cx + reach + G.hw, G.z, G.z, code, G.slides != 0);
    }
    for (int i = 0; i < L.barrierCount; ++i) {
        const Barrier& B = L.barriers[i];
        m.cover(B.x0, B.x1, B.z, B.z, kFpWall, false);
    }
    for (int i = 0; i < L.bumperCount; ++i) {
        const Bumper& B = L.bumpers[i];
        m.cover(B.x - B.r, B.x + B.r, B.z - B.r, B.z + B.r, kFpBumper, false);
    }
    for (int i = 0; i < L.snufferCount; ++i) {
        const Snuffer& S = L.snuffers[i];
        m.cover(S.xa - 0.6f, S.xb + 0.6f, S.z, S.z, kFpSnuffer, false);
    }
    // Geodes: the path each drifts down, and the boons offered in the order they come.
    int order[kMaxGeodes];
    const int ng = minv(L.geodeCount, kMaxGeodes);
    for (int i = 0; i < ng; ++i) {
        const Geode& G = L.geodes[i];
        const float z0 = G.z0 > 0 ? G.z0 : L.len - 3.0f;
        m.cover(G.x - G.radius - G.amp, G.x + G.radius + G.amp, 4.0f, z0, kFpGeode, G.amp > 0);
        order[i] = i;
    }
    for (int a = 0; a < ng; ++a)
        for (int b = a + 1; b < ng; ++b)
            if (L.geodes[order[b]].t < L.geodes[order[a]].t) {
                const int t = order[a];
                order[a] = order[b];
                order[b] = t;
            }
    for (int i = 0; i < ng && fp.boonCount < 16; ++i) fp.boons[fp.boonCount++] = L.geodes[order[i]].boon;
    return fp;
}

float mapLikeness(const Fingerprint& a, const Fingerprint& b) {
    int used = 0, same = 0, sameMirror = 0;
    for (int band = 0; band < kFpBands; ++band)
        for (int c = 0; c < kFpCols; ++c) {
            const u8 x = a.cells[band * kFpCols + c];
            const u8 y = b.cells[band * kFpCols + c];
            const u8 ym = b.cells[band * kFpCols + (kFpCols - 1 - c)];
            // A cell counts when it is not empty in at least one of the two maps (as compared).
            used += (x || y) ? 1 : 0;
            same += (x || y) && x == y ? 1 : 0;
            sameMirror += (x || ym) && x == ym ? 1 : 0;
        }
    // The mirrored comparison has its own count of used cells.
    int usedMirror = 0;
    for (int band = 0; band < kFpBands; ++band)
        for (int c = 0; c < kFpCols; ++c)
            usedMirror += (a.cells[band * kFpCols + c] || b.cells[band * kFpCols + (kFpCols - 1 - c)]) ? 1 : 0;
    const float m0 = used ? float(same) / float(used) : 1.0f;
    const float m1 = usedMirror ? float(sameMirror) / float(usedMirror) : 1.0f;
    return maxv(m0, m1);
}

float likeness(const Fingerprint& a, const Fingerprint& b) {
    // J: the Jaccard index of the two boon sets (1 when both are empty).
    u32 sa = 0, sb = 0;
    for (int i = 0; i < a.boonCount; ++i) sa |= 1u << (a.boons[i] & 31);
    for (int i = 0; i < b.boonCount; ++i) sb |= 1u << (b.boons[i] & 31);
    auto bits = [](u32 v) {
        int n = 0;
        for (; v; v &= v - 1) ++n;
        return n;
    };
    const int uni = bits(sa | sb);
    const float j = uni ? float(bits(sa & sb)) / float(uni) : 1.0f;
    return 0.20f * float(a.kind == b.kind) + 0.15f * float(a.recipe == b.recipe) + 0.10f * float(a.plan == b.plan) +
           0.05f * float(a.lengthClass == b.lengthClass) + 0.10f * j + 0.40f * mapLikeness(a, b);
}

float likenessLimit(int distance) { return distance <= 12 ? 0.45f : distance <= 40 ? 0.60f : 0.85f; }

bool breaksRule(const Fingerprint& a, const Fingerprint& b, int distance, const char** why) {
    const char* w = nullptr;
    if (likeness(a, b) > likenessLimit(distance)) w = "too alike";
    else if (distance <= kRecipeWindow && a.recipe == b.recipe) w = "the same recipe";
    else if (distance <= kRecipeKindWindow && a.recipe == b.recipe && a.kind == b.kind) w = "the same recipe and kind";
    else if (distance <= kBoonListWindow && a.boonCount > 0 && a.boonCount == b.boonCount &&
             std::equal(a.boons, a.boons + a.boonCount, b.boons))
        w = "the same boon list";
    if (why) *why = w;
    return w != nullptr;
}

int varietyReport(int from, int to, bool pairs) {
    from = maxv(1, from);
    if (to < from) return 2;
    const int n = to - from + 1;
    std::vector<Fingerprint> fps(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i) fps[size_t(i)] = fingerprintOf(levelFor(from + i));

    struct Class {
        const char* name;
        int lo, hi;
        long pairs = 0, above = 0;
        float worst = -1;
        int wa = 0, wb = 0;
        std::vector<float> values;
    } classes[3] = {{"1-12", 1, 12}, {"13-40", 13, 40}, {"41+", 41, 1 << 30}};
    struct Pair {
        int a, b;
        float like, limit, map;
    };
    std::vector<Pair> over;
    long recipeHits = 0, recipeKindHits = 0, boonHits = 0;
    for (int i = 0; i < n; ++i)
        for (int j = i + 1; j < n; ++j) {
            const int d = j - i;
            const Fingerprint &a = fps[size_t(i)], &b = fps[size_t(j)];
            const float like = likeness(a, b), limit = likenessLimit(d);
            Class& c = classes[d <= 12 ? 0 : d <= 40 ? 1 : 2];
            ++c.pairs;
            if (like > limit) ++c.above;
            if (like > c.worst) c.worst = like, c.wa = from + i, c.wb = from + j;
            if (d <= 40) c.values.push_back(like);
            if (d <= 40) over.push_back({from + i, from + j, like, limit, mapLikeness(a, b)});
            if (d <= kRecipeWindow && a.recipe == b.recipe) ++recipeHits;
            if (d <= kRecipeKindWindow && a.recipe == b.recipe && a.kind == b.kind) ++recipeKindHits;
            if (d <= kBoonListWindow && a.boonCount > 0 && a.boonCount == b.boonCount && std::equal(a.boons, a.boons + a.boonCount, b.boons))
                ++boonHits;
        }
    std::printf("# Variety report, levels %d to %d\n\n", from, to);
    std::printf("| Levels apart | Pairs | Limit | Above it | Share | Likeness p10 / p50 / p90 | Most alike pair |\n|---|---|---|---|---|---|---|\n");
    long broken = 0;
    for (Class& c : classes) {
        broken += c.above;
        char dist[64] = "-";
        if (!c.values.empty()) {
            std::sort(c.values.begin(), c.values.end());
            auto at = [&](float q) { return c.values[size_t(q * float(c.values.size() - 1))]; };
            std::snprintf(dist, sizeof dist, "%.2f / %.2f / %.2f", double(at(0.1f)), double(at(0.5f)), double(at(0.9f)));
        }
        std::printf("| %s | %ld | %.2f | %ld | %.1f%% | %s | %d and %d (%.2f) |\n", c.name, c.pairs, double(likenessLimit(c.lo)), c.above,
                    c.pairs ? 100.0 * double(c.above) / double(c.pairs) : 0.0, dist, c.wa, c.wb, double(c.worst));
    }
    broken += recipeHits + recipeKindHits + boonHits;
    std::printf("\nCounts: a recipe twice within %d levels: %ld; a (recipe, kind) pair twice within %d: %ld; a boon list twice within %d: "
                "%ld.\n",
                kRecipeWindow + 1, recipeHits, kRecipeKindWindow + 1, recipeKindHits, kBoonListWindow + 1, boonHits);

    // The longest runs of one kind, one recipe, and both.
    auto longest = [&](auto same, int& at) {
        int best = 1, run = 1;
        at = from;
        for (int i = 1; i < n; ++i) {
            run = same(fps[size_t(i - 1)], fps[size_t(i)]) ? run + 1 : 1;
            if (run > best) best = run, at = from + i - run + 1;
        }
        return best;
    };
    int ka = 0, ra = 0, ba = 0;
    const int kr = longest([](const Fingerprint& x, const Fingerprint& y) { return x.kind == y.kind; }, ka);
    const int rr = longest([](const Fingerprint& x, const Fingerprint& y) { return x.recipe == y.recipe; }, ra);
    const int br = longest([](const Fingerprint& x, const Fingerprint& y) { return x.kind == y.kind && x.recipe == y.recipe; }, ba);
    std::printf("Longest run of one kind: %d (from level %d); of one recipe: %d (from %d); of one kind and one recipe: %d (from %d).\n", kr,
                ka, rr, ra, br, ba);

    // Per hundred levels: kinds, recipes, plans, length classes, twists and boons.
    std::printf("\n| Levels | Kinds | Recipes | Plans | Lengths | Twists | Boons offered |\n|---|---|---|---|---|---|---|\n");
    for (int h0 = from; h0 <= to; h0 += 100) {
        const int h1 = minv(to, h0 + 99);
        int kinds[kKindCount] = {}, lengths[kLengthCount] = {}, recipes[64] = {}, plans[32] = {}, twists[32] = {}, boons[32] = {};
        for (int l = h0; l <= h1; ++l) {
            const Fingerprint& f = fps[size_t(l - from)];
            ++kinds[f.kind % kKindCount];
            ++lengths[f.lengthClass % kLengthCount];
            ++recipes[f.recipe & 63];
            ++plans[f.plan & 31];
            for (int t = 0; t < f.twistCount; ++t) ++twists[f.twists[t] & 31];
            for (int b = 0; b < f.boonCount; ++b) ++boons[f.boons[b] & 31];
        }
        auto list = [](char* out, size_t cap, const int* v, int count, const char* (*name)(int)) {
            size_t o = 0;
            out[0] = 0;
            for (int i = 0; i < count && o + 1 < cap; ++i)
                if (v[i]) o += size_t(std::snprintf(out + o, cap - o, "%s%s %d", o ? ", " : "", name(i), v[i]));
            if (!o) std::snprintf(out, cap, "-");
        };
        char k[256], r[512], p[256], le[128], tw[256], bo[256];
        list(k, sizeof k, kinds, kKindCount, kindName);
        list(r, sizeof r, recipes, 64, recipeName);
        list(p, sizeof p, plans, 32, planName);
        list(le, sizeof le, lengths, kLengthCount, lengthClassName);
        list(tw, sizeof tw, twists, 32, twistName);
        list(bo, sizeof bo, boons, 32, boonName);
        std::printf("| %d-%d | %s | %s | %s | %s | %s | %s |\n", h0, h1, k, r, p, le, tw, bo);
    }

    if (pairs) {
        std::sort(over.begin(), over.end(), [](const Pair& x, const Pair& y) {
            const float dx = x.like - x.limit, dy = y.like - y.limit;
            return dx != dy ? dx > dy : x.a < y.a;
        });
        std::printf("\nThe twenty pairs within 40 levels furthest over their limit:\n\n| Level | Level | Apart | Likeness | Limit | Map M |\n"
                    "|---|---|---|---|---|---|\n");
        for (size_t i = 0; i < over.size() && i < 20; ++i)
            std::printf("| %d | %d | %d | %.2f | %.2f | %.2f |\n", over[i].a, over[i].b, over[i].b - over[i].a, double(over[i].like),
                        double(over[i].limit), double(over[i].map));
    }
    std::printf("\nvariety: %s (%ld lines broken)\n", broken ? "FAILED" : "ok", broken);
    return broken ? 1 : 0;
}

}  // namespace gt::sim

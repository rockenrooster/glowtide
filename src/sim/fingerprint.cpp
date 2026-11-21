#include "sim/fingerprint.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <thread>
#include <vector>

#include "sim/level_build.h"

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

// How many levels of a kind the calendar puts before a level (the table up to the last baked level, then counted).
int kindOrdinal(int index, int kind) {
    static const std::vector<u16> ord = [] {
        std::vector<u16> v(size_t(kLastBaked + 1), 0);
        int seen[kKindCount] = {};
        for (int l = 1; l <= kLastBaked; ++l) v[size_t(l)] = u16(seen[kindFor(l)]++);
        return v;
    }();
    if (index >= 1 && index <= kLastBaked && kindFor(index) == kind) return ord[size_t(index)];
    int n = 0;
    for (int l = 1; l < index; ++l) n += kindFor(l) == kind ? 1 : 0;
    return n;
}

}  // namespace

Fingerprint fingerprintOf(const LevelDef& L) {
    Fingerprint fp;
    fp.level = u16(L.index);
    fp.kind = L.kind;
    fp.kindOrdinal = u16(kindOrdinal(L.index, L.kind));
    fp.recipe = isHandMade(L.index) ? u8(128 + minv(L.index, 127)) : L.recipe;
    int choices = 0;
    for (int i = 0; i < recipeCount(); ++i) choices += (kRecipes[i].kinds >> L.kind & 1u) && L.index >= kRecipes[i].from ? 1 : 0;
    fp.choices = u8(clampv(choices, 1, 255));
    fp.plan = L.plan;
    fp.lengthClass = L.lengthClass;
    for (int k = 0; k < L.twistCount && fp.twistCount < 4; ++k)  // the set of twists (SECOND WIND, everywhere from 23, aside)
        if (L.twists[k].type != kTwistSecondWind) fp.twists[fp.twistCount++] = L.twists[k].type;
    Map m{fp, 4.0f, maxv(1.0f, L.len - 8.0f) / float(kFpBands)};
    for (int i = 0; i < L.gateCount; ++i) {
        const Gate& G = L.gates[i];
        const float reach = G.slides ? G.amp : G.breath > 0 ? 0.2f * G.hw : 0.0f;  // a breath opens to 120%
        const u8 code = G.kind == kGateMul ? u8(kFpMul + clampv(int(G.k), 2, 6)) : u8(kFpDiv);
        m.cover(G.cx - reach - G.hw, G.cx + reach + G.hw, G.z, G.z, code, G.slides != 0 || G.breath > 0);
    }
    for (int i = 0; i < L.barrierCount; ++i) {
        const Barrier& B = L.barriers[i];
        m.cover(B.x0 - B.amp, B.x1 + B.amp, B.z, B.z, kFpWall, B.amp > 0);
    }
    for (int i = 0; i < L.emitterCount; ++i) {  // side doors, on the rail
        const Emitter& E = L.emitters[i];
        if (E.side) m.cover(float(E.side) * (kHW - 0.6f), float(E.side) * (kHW - 0.6f), E.z - 0.6f, E.z + 0.6f, kFpDoor, false);
    }
    for (int i = 0; i < L.bumperCount; ++i) {
        const Bumper& B = L.bumpers[i];
        m.cover(B.x - B.r, B.x + B.r, B.z - B.r, B.z + B.r, kFpBumper, false);
    }
    for (int i = 0; i < L.snufferCount; ++i) {
        const Snuffer& S = L.snuffers[i];
        m.cover(S.xa - 0.6f, S.xb + 0.6f, S.z, S.z, kFpSnuffer, false);
    }
    // Geodes: where each appears, as a thumbnail shows it (VARIETY_TECH 6.1 said its path: the paths drew a column down
    // the whole lane for every geode, so any two levels with geodes on the same side looked alike), and the boons offered
    // in the order they come.
    int order[kMaxGeodes];
    const int ng = minv(L.geodeCount, kMaxGeodes);
    for (int i = 0; i < ng; ++i) {
        const Geode& G = L.geodes[i];
        const float z0 = G.z0 > 0 ? G.z0 : L.len - 3.0f;
        m.cover(G.x - G.radius - G.amp, G.x + G.radius + G.amp, z0, z0, kFpGeode, G.amp > 0);
        order[i] = i;
    }
    for (int a = 0; a < ng; ++a)
        for (int b = a + 1; b < ng; ++b)
            if (L.geodes[order[b]].t < L.geodes[order[a]].t) {
                const int t = order[a];
                order[a] = order[b];
                order[b] = t;
            }
    for (int i = 0; i < ng && fp.boonCount < 16; ++i) {  // SECOND WIND's WARD is on every level from 23: not an offer
        const Geode& G = L.geodes[order[i]];
        if (!(G.twist && L.twists[G.twist - 1].type == kTwistSecondWind)) fp.boons[fp.boonCount++] = G.boon;
    }
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
    return kLikeKind * float(a.kind == b.kind) + kLikeRecipe * float(a.recipe == b.recipe) + kLikePlan * float(a.plan == b.plan) +
           kLikeLength * float(a.lengthClass == b.lengthClass) + kLikeBoons * j + kLikeMap * mapLikeness(a, b);
}

float likenessLimit(int distance) { return distance <= 12 ? kLimitNear : distance <= 40 ? kLimitMid : kLimitFar; }

namespace {
// The recipe lines (VARIETY_DESIGN 11, as tuned): a is the earlier level, b the later.
int turns(int choices) { return clampv(choices - 2, choices > 1 ? 1 : 0, 1 << 20); }  // 0 with one recipe to draw
bool sameRecipeTooSoon(const Fingerprint& a, const Fingerprint& b, int distance) {
    return a.recipe == b.recipe && distance <= minv(kRecipeWindow, turns(b.choices));
}
bool sameRecipeKindTooSoon(const Fingerprint& a, const Fingerprint& b, int distance) {
    // A (recipe, kind) pair comes back after two thirds of the kind's recipes have had a turn (Siege has some eighteen
    // levels in every thirty and twenty recipes: all distinct would leave a draw two or three recipes to land on).
    return a.recipe == b.recipe && a.kind == b.kind && distance <= kRecipeKindWindow &&
           int(b.kindOrdinal) - int(a.kindOrdinal) <= minv(turns(b.choices), maxv(1, 2 * int(b.choices) / 3));
}
// From level 21: before it a level offers one or two geodes from a pool of one to three boons, so lists repeat by force.
bool sameBoonListTooSoon(const Fingerprint& a, const Fingerprint& b, int distance) {
    return b.level >= 21 && distance <= kBoonListWindow && a.boonCount > 0 && a.boonCount == b.boonCount &&
           std::equal(a.boons, a.boons + a.boonCount, b.boons);
}
}  // namespace

bool breaksRule(const Fingerprint& a, const Fingerprint& b, int distance, const char** why) {
    const char* w = nullptr;
    if (likeness(a, b) > likenessLimit(distance)) w = "too alike";
    else if (sameRecipeTooSoon(a, b, distance)) w = "the same recipe";
    else if (sameRecipeKindTooSoon(a, b, distance)) w = "the same recipe and kind";
    else if (sameBoonListTooSoon(a, b, distance))
        w = "the same boon list";
    if (why) *why = w;
    return w != nullptr;
}

int varietyReport(int from, int to, bool pairs) {
    from = maxv(1, from);
    if (to < from) return 2;
    std::vector<Fingerprint> all(size_t(to + 1));
    for (int l = from; l <= to; ++l) all[size_t(l)] = fingerprintOf(levelFor(l));
    return varietyReportOn(all, from, to, pairs);
}

// The report on fingerprints already made (fps[l] for l in from..to): the baked table's, or the first pass's choices.
int varietyReportOn(const std::vector<Fingerprint>& all, int from, int to, bool pairs) {
    const int n = to - from + 1;
    std::vector<Fingerprint> fps(all.begin() + from, all.begin() + to + 1);

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
    struct Hit {
        int a, b;
        const char* what;
    };
    std::vector<Hit> hits;  // pairs breaking a recipe or boon line
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
            const bool hr = sameRecipeTooSoon(a, b, d), hk = sameRecipeKindTooSoon(a, b, d), hb = sameBoonListTooSoon(a, b, d);
            recipeHits += hr;
            recipeKindHits += hk;
            boonHits += hb;
            if (hr || hk || hb) hits.push_back({from + i, from + j, hr ? "the same recipe" : hk ? "the same recipe and kind" : "the same boon list"});
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
    for (const Hit& h : hits) std::printf("Rule broken by levels %d and %d: %s.\n", h.a, h.b, h.what);

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
        int kinds[kKindCount] = {}, lengths[kLengthCount] = {}, recipes[256] = {}, plans[32] = {}, twists[32] = {}, boons[32] = {};
        for (int l = h0; l <= h1; ++l) {
            const Fingerprint& f = fps[size_t(l - from)];
            ++kinds[f.kind % kKindCount];
            ++lengths[f.lengthClass % kLengthCount];
            ++recipes[f.recipe >= 128 ? 255 : f.recipe];  // hand-made levels together
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
        char k[256], r[1024], p[256], le[128], tw[256], bo[256];
        list(k, sizeof k, kinds, kKindCount, kindName);
        list(r, sizeof r, recipes, 256, [](int i) { return i == 255 ? "by hand" : recipeName(i); });
        list(p, sizeof p, plans, 32, planName);
        list(le, sizeof le, lengths, kLengthCount, lengthClassName);
        list(tw, sizeof tw, twists, 32, twistName);
        list(bo, sizeof bo, boons, 32, boonName);
        std::printf("| %d-%d | %s | %s | %s | %s | %s | %s |\n", h0, h1, k, r, p, le, tw, bo);
    }

    // The variety contract (VARIETY_PLAN V4's exit checks): per hundred levels, Holdout, Colossus and Twins each within three
    // points of VARIETY_DESIGN 6's share of the levels since its introduction (Siege is what is left: with festivals on 5,
    // bosses on 0 and no other kind twice in a row it is about 60%, not 55%; festivals and bosses stand where the calendar
    // puts them); every recipe of the catalogue allowed from the hundred's start that six levels or more of it could have
    // drawn at least twice, one introduced inside it or with fewer such levels at least once (a recipe made for one kind
    // has only that kind's levels), none more than twelve times; no kind but Siege twice in a row; and up to level 100
    // something new (a kind, a recipe, a plan, a twist, a boon, or a level laid out by hand) at least every five levels.
    long contract = 0;
    for (int h0 = from; h0 <= to; h0 += 100) {
        const int h1 = minv(to, h0 + 99), size = h1 - h0 + 1;
        int kinds[kKindCount] = {}, recipes[256] = {};
        for (int l = h0; l <= h1; ++l) ++kinds[fps[size_t(l - from)].kind % kKindCount], ++recipes[fps[size_t(l - from)].recipe];
        static const int shareKind[3] = {kHoldout, kColossus, kTwins};
        static const float share[3] = {0.10f, 0.08f, 0.07f};
        static const int intro[3] = {22, 32, 42};
        for (int k = 0; k < 3; ++k) {
            const float want = share[k] * float(maxv(0, h1 - maxv(h0, intro[k]) + 1)) / float(size), got = float(kinds[shareKind[k]]) / float(size);
            if (std::fabs(got - want) > 0.03f + 1e-4f) {
                std::printf("contract: levels %d-%d: %s %.1f%% of the levels, VARIETY_DESIGN 6 wants %.1f%%\n", h0, h1, kindName(shareKind[k]),
                            double(got * 100), double(want * 100));
                ++contract;
            }
        }
        for (int rc = 0; rc < recipeCount(); ++rc) {
            int could = 0;  // levels of the hundred laid out by the calendar's draw, of a kind the recipe serves, from its first level
            for (int l = maxv(h0, kRecipes[rc].from); l <= h1; ++l)
                could += !isHandMade(l) && setPieceAt(l) < 0 && introRecipe(l) < 0 && (kRecipes[rc].kinds >> fps[size_t(l - from)].kind & 1u) ? 1 : 0;
            const int least = could >= 6 && kRecipes[rc].from <= h0 ? 2 : could > 0 ? 1 : 0;
            if (recipes[rc] < least || recipes[rc] > 12) {
                std::printf("contract: levels %d-%d: recipe %s %d times (%d to 12)\n", h0, h1, recipeName(rc), recipes[rc], least);
                ++contract;
            }
        }
    }
    for (int l = from + 1; l <= to; ++l)
        if (fps[size_t(l - from)].kind != kSiege && fps[size_t(l - 1 - from)].kind != kSiege) {
            std::printf("contract: levels %d and %d are both %s and %s\n", l - 1, l, kindName(fps[size_t(l - 1 - from)].kind),
                        kindName(fps[size_t(l - from)].kind));
            ++contract;
        }
    if (from == 1) {
        u32 seenKinds = 0, seenPlans = 0, seenTwists = 0, seenBoons = 0;
        bool seenRecipe[256] = {};
        int lastNew = 0, worstGap = 0, worstAt = 0;
        for (int l = 1; l <= minv(to, 100); ++l) {
            const Fingerprint& f = fps[size_t(l - from)];
            bool fresh = isHandMade(l) || !(seenKinds >> f.kind & 1u) || !seenRecipe[f.recipe] || !(seenPlans >> f.plan & 1u);
            for (int t = 0; t < f.twistCount; ++t) fresh = fresh || !(seenTwists >> f.twists[t] & 1u), seenTwists |= 1u << f.twists[t];
            for (int b = 0; b < f.boonCount; ++b) fresh = fresh || !(seenBoons >> f.boons[b] & 1u), seenBoons |= 1u << f.boons[b];
            seenKinds |= 1u << f.kind;
            seenPlans |= 1u << f.plan;
            seenRecipe[f.recipe] = true;
            if (!fresh) continue;
            if (l - lastNew > worstGap) worstGap = l - lastNew, worstAt = lastNew;
            lastNew = l;
        }
        if (minv(to, 100) - lastNew > worstGap) worstGap = minv(to, 100) - lastNew, worstAt = lastNew;
        std::printf("Something new up to level 100: at most %d levels apart (after level %d).\n", worstGap, worstAt);
        if (worstGap > 5) ++contract;
    }
    std::printf("Variety contract: %ld lines broken.\n", contract);
    broken += contract;

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

u64 seedFor(int level, int tries) { return defaultSeed(level) + u64(tries) * 1000003u; }

// Whether a candidate for `level` keeps the rule against every fixed level; `broken` counts the pairs it breaks (it stops
// counting at stopAt).
bool ruleHolds(const std::vector<Fingerprint>& fps, const std::vector<bool>& fixed, int level, const Fingerprint& f, int* broken,
               const char** why, int stopAt) {
    int n = 0;
    for (int j = 1; j < int(fps.size()) && n < stopAt; ++j) {
        if (j == level || !fixed[size_t(j)]) continue;
        const char* w = nullptr;
        const bool later = j > level;
        if (breaksRule(later ? f : fps[size_t(j)], later ? fps[size_t(j)] : f, later ? j - level : level - j, &w)) {
            if (n == 0 && why) *why = w;
            ++n;
        }
    }
    if (broken) *broken = n;
    return n == 0;
}

std::vector<LayoutChoice> chooseLayouts(int from, int to, int keepTo, std::vector<Fingerprint>& fps, int maxSeeds, LayoutTest viable,
                                        int jobs) {
    const int last = maxv(to, keepTo);
    fps.assign(size_t(last + 1), Fingerprint{});
    std::vector<bool> fixed(size_t(last + 1), false);
    for (int l = 1; l <= last; ++l)
        if (l < from || l > to) fps[size_t(l)] = fingerprintOf(levelFor(l)), fixed[size_t(l)] = true;
    std::vector<LayoutChoice> out(size_t(maxv(0, to - from + 1)));
    std::vector<int> next(out.size(), 0);  // where a level's search goes on when the pass steps back to it
    char lintWhy[96];
    int stepsBack = 0;
    // A level's candidates, in the pass's order of preference: for a level whose recipe is forced (by hand, an
    // introduction, a set piece) its seeds; otherwise its recipes, the one used least on the level's kind in the last thirty
    // levels first (the (recipe, kind) window, so a later level is not left without a recipe it may take), then the one its
    // hundred has used least (two to twelve a hundred: the contract), then the calendar's weight, kSeedsPerRecipe seeds of
    // each. Pass 1 takes the first that passes the lints, keeps the rule and passes `viable`.
    constexpr int kSeedsPerRecipe = 8;
    auto order = [&](int l, int* recipes) {
        const int kind = kindFor(l);
        int n = 0;
        float key[64];
        for (int rc = 0; rc < recipeCount() && n < 64; ++rc) {
            const float w = recipeWeight(l, kind, rc);
            if (w <= 0) continue;
            int window = 0, hundred = 0;
            for (int j = maxv(1, l - 30); j < l; ++j) window += fixed[size_t(j)] && fps[size_t(j)].recipe == rc && fps[size_t(j)].kind == kind ? 1 : 0;
            for (int j = ((l - 1) / 100) * 100 + 1; j < l; ++j) hundred += fixed[size_t(j)] && fps[size_t(j)].recipe == rc ? 1 : 0;
            recipes[n] = rc;
            key[n] = float(window * 1000 + hundred * 10) - w + float(hash64(u64(l) * 131u + u64(rc)) % 997u) * 1e-4f;
            ++n;
        }
        for (int a = 1; a < n; ++a)  // a few dozen: insertion sort
            for (int b = a; b > 0 && key[b] < key[b - 1]; --b) {
                std::swap(key[b], key[b - 1]);
                std::swap(recipes[b], recipes[b - 1]);
            }
        return n;
    };
    for (int l = from; l <= to;) {
        const size_t i = size_t(l - from);
        LayoutChoice& c = out[i];
        const bool forced = forcedRecipe(l);
        int recipes[64], nr = 0;
        if (!forced) nr = order(l, recipes);
        const int seeds = isHandMade(l) ? 1 : forced ? maxSeeds : nr * kSeedsPerRecipe;
        auto candidate = [&](int k, int& recipe, int& t) {
            recipe = forced ? -1 : recipes[k / kSeedsPerRecipe];
            t = forced ? k : k % kSeedsPerRecipe;
        };
        struct Cand {
            int recipe, t;
            Fingerprint f;
        };
        bool found = false, settled = false;
        while (!settled) {
            // Up to six candidates that pass the lints and keep the rule, in order.
            Cand cands[6];
            int nc = 0;
            for (int k = next[i]; k < seeds && nc < 6; ++k) {
                int recipe, t;
                candidate(k, recipe, t);
                const LevelDef def = layoutFor(l, seedFor(l, t), recipe);
                ++c.looked;
                next[i] = k + 1;
                if (!isHandMade(l) && !lintLevel(def, l, lintWhy, sizeof lintWhy)) {
                    ++c.lintFails;
                    continue;
                }
                const Fingerprint f = fingerprintOf(def);
                if (!ruleHolds(fps, fixed, l, f, nullptr, nullptr, 1)) continue;
                cands[nc++] = {recipe, t, f};
            }
            if (nc == 0) break;
            int pick = 0;
            if (viable && !isHandMade(l)) {
                // The viability test on `jobs` candidates at a time, in their order; none passing, the first of the first six.
                pick = -1;
                for (int k0 = 0; k0 < nc && pick < 0; k0 += maxv(1, jobs)) {
                    const int k1 = minv(nc, k0 + maxv(1, jobs));
                    bool ok[6] = {};
                    std::vector<std::thread> pool;
                    for (int k = k0; k < k1; ++k)
                        pool.emplace_back([&, k] { ok[k] = viable(layoutFor(l, seedFor(l, cands[k].t), cands[k].recipe)); });
                    for (std::thread& th : pool) th.join();
                    for (int k = k0; k < k1 && pick < 0; ++k)
                        if (ok[k]) pick = k;
                        else ++c.unviable;
                }
                if (pick < 0) {
                    if (!found) {  // keep the first batch's best as the fallback, and look further
                        fps[size_t(l)] = cands[0].f;
                        c.tries = cands[0].t;
                        c.recipe = cands[0].recipe;
                        found = true;
                    }
                    if (next[i] < seeds) continue;
                    settled = true;
                    break;
                }
            }
            fps[size_t(l)] = cands[pick].f;
            c.tries = cands[pick].t;
            c.recipe = cands[pick].recipe;
            found = settled = true;
        }
        if (found) {
            c.broken = 0;
            c.why[0] = 0;
            fixed[size_t(l)] = true;
            ++l;
            continue;
        }
        // None keeps the rule against what came before: an earlier choice left no room (a recipe the window still holds,
        // say). Step back to the generated level before (up to eight levels back), take its next seed, and come forward again.
        int back = l - 1;
        while (back >= from && isHandMade(back)) --back;
        if (back >= from && l - back <= 8 && stepsBack < 4000) {
            ++stepsBack;
            for (int k = back; k <= l; ++k) {
                fixed[size_t(k)] = false;
                if (k > back) next[size_t(k - from)] = 0;
            }
            l = back;
            continue;
        }
        // Still none: the candidate that breaks the fewest pairs, said in the report.
        int bestBroken = 1 << 30;
        Fingerprint bestFp = fingerprintOf(layoutFor(l, seedFor(l, 0), forced ? -1 : recipes[0]));
        for (int k = 0; k < seeds; ++k) {
            int recipe, t;
            candidate(k, recipe, t);
            const LevelDef def = layoutFor(l, seedFor(l, t), recipe);
            if (!isHandMade(l) && !lintLevel(def, l, lintWhy, sizeof lintWhy)) continue;
            const Fingerprint f = fingerprintOf(def);
            int broken = 0;
            const char* w = nullptr;
            ruleHolds(fps, fixed, l, f, &broken, &w, bestBroken);
            if (broken < bestBroken) {
                bestBroken = broken;
                bestFp = f;
                c.tries = t;
                c.recipe = recipe;
                c.broken = broken;
                std::snprintf(c.why, sizeof c.why, "%s", w ? w : "");
            }
        }
        fps[size_t(l)] = bestFp;
        fixed[size_t(l)] = true;
        next[i] = seeds;
        ++l;
    }
    return out;
}

int layoutsReport(int from, int to) {
    std::vector<Fingerprint> fps;
    const auto picks = chooseLayouts(maxv(1, from), to, 0, fps);
    int none = 0, many = 0, lintOnly = 0;
    long looked = 0;
    std::printf("| Level | Kind | Recipe | Seed number | Seeds looked at | Lint fails | Pairs broken | First line broken |\n|---|---|---|---|---|---|---|---|\n");
    for (size_t i = 0; i < picks.size(); ++i) {
        const LayoutChoice& c = picks[i];
        const int l = maxv(1, from) + int(i);
        looked += c.looked;
        none += c.broken > 0 ? 1 : 0;
        many += c.looked > 16 ? 1 : 0;
        lintOnly += c.lintFails == c.looked ? 1 : 0;
        if (c.broken > 0 || c.looked > 16)
            std::printf("| %d | %s | %s | %d | %d | %d | %d | %s |\n", l, kindName(fps[size_t(l)].kind), recipeName(fps[size_t(l)].recipe), c.tries,
                        c.looked, c.lintFails, c.broken, c.why);
    }
    std::printf("\nlayouts: %d levels, %.1f seeds looked at a level, %d needed more than 16, %d found none that keeps the rule, %d none "
                "that passes the lints\n",
                int(picks.size()), double(looked) / double(maxv<size_t>(1, picks.size())), many, none, lintOnly);
    std::printf("\n");
    varietyReportOn(fps, maxv(1, from), to, true);
    return none || lintOnly ? 1 : 0;
}

}  // namespace gt::sim

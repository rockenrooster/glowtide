#pragma once
// Runs with bots, the acceptance tables and the level bake (GAME_DESIGN 7.4, 8; TECH_SPEC 4.8).

#include "sim/bots.h"
#include "sim/world.h"

namespace gt::sim {

struct RunOptions {
    float seconds = 60;
    bool unbreakable = false;
    bool enemyOff = false;
    bool measureTicks = false;
    bool validate = false;  // check the world's invariants every tick (soak)
};

struct RunResult {
    Result result = Result::None;
    float time = 0;
    int core = 0, spire = 0;
    i64 produced = 0, cancelled = 0, delivered = 0, peakTide = 0;
    int peakWisps = 0, peakGlooms = 0;
    float lowestGloomZ = 0;
    float tickUs[3] = {};  // p50, p95, max
    u64 hash = 0;
    bool invalid = false;
    char why[96] = "";
    float share() const { return produced ? float(cancelled) / float(produced) : 0.0f; }
};

RunResult runLevel(const LevelDef& def, BotKind bot, u64 noiseSeed, const RunOptions& ro);
LevelDef benchFloodLevel();

// --sim table: a markdown table of results per level and bot.
int bakeTable(int from, int to, const char* bots, int seeds);
// --sim soak: one pass over the levels per bot, invariants checked every tick.
int soak(int from, int to, const char* bots);
// --sim accept: GAME_DESIGN 8's acceptance table for the hand-made levels (and 7.4's lines for 2-5).
int acceptIntro(int from, int to);
// --sim flux: GAME_DESIGN 5.9's four findings, checked with the real simulation on level 5's layout and on a
// three-row x30 layout (the cases of docs/concept/flux_model.py). Prints markdown.
int fluxFindings();
// --bake-levels: GAME_DESIGN 7.4 for levels from..to; writes levels_baked.h and the report.
int bakeLevels(int from, int to, int jobs, int part, int parts, const char* out, const char* report);

}  // namespace gt::sim

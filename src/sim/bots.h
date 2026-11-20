#pragma once
// Bots (GAME_DESIGN 8, VARIETY_DESIGN 12.1): they hold the thumb through the same Input a player produces, and see only
// the lane (a geode's boon and health are drawn, so they may read them).

#include "core/rng.h"
#include "sim/world.h"

namespace gt::sim {

// greedy: the competent player, who weighs a geode's cost against its boon; podless: greedy that gives geodes no worth
// (a geode on its line is only a wall); hungry: aims at the nearest geode whenever one is on the lane.
enum class BotKind { Greedy, Naive, Sweep, Idle, Podless, Hungry };
bool botFromName(const char* name, BotKind& out);
const char* botName(BotKind k);

class Bot {
public:
    void init(BotKind kind, u64 noiseSeed);
    Input think(const World& w);  // once a tick
    BotKind kind = BotKind::Greedy;
    // What a straight stream from x delivers in the next 8 s (power), with the time geodes, plates and walls on the line
    // take out of it; with `worth`, plus what the boons it would open are worth over the rest of the level, at
    // `bestRate` (power a second of the best plain line).
    float score(const World& w, float x, bool worth = false, float bestRate = 0) const;
    int nearestGeode(const World& w, float* x) const;  // the alive geode nearest the lanterns, and where to aim at it
    int geodeOnLine(const World& w, float x) const;    // the first alive geode a stream from x meets, or -1

private:
    Rng rng_;
    float hand_ = 0;       // where the thumb has the launcher go
    float aim_ = 0;        // the decided position (applied after the reaction delay)
    float applied_ = 0;    // the decision the hand is moving toward now
    int nextDecision_ = 0;
    float queued_[16] = {};
    int queuedTick_[16] = {};
    int qHead_ = 0, qCount_ = 0;
    int defendUntil_ = -1;
    float sweepDir_ = 1;
    int commit_ = -1;    // a geode greedy has started on: it stays on it unless another line is far better
    int commitDeadline_ = 0;  // the tick by which the stream must have taken a tenth of its health
    i32 commitHealth_ = 0;
    u32 givenUp_ = 0;    // geodes greedy committed to and could not crack (no worth from them again)
    float rate_ = 0;     // the best plain line's rate, smoothed (sliders make it jump)
};

}  // namespace gt::sim

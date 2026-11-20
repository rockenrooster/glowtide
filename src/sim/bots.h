#pragma once
// Bots (GAME_DESIGN 8): they hold the thumb through the same Input a player produces, and see only the lane.

#include "core/rng.h"
#include "sim/world.h"

namespace gt::sim {

enum class BotKind { Greedy, Naive, Sweep, Idle };
bool botFromName(const char* name, BotKind& out);
const char* botName(BotKind k);

class Bot {
public:
    void init(BotKind kind, u64 noiseSeed);
    Input think(const World& w);  // once a tick
    BotKind kind = BotKind::Greedy;
    // The greedy bot's estimate of what a straight stream from x delivers in the next 8 s (power).
    float score(const World& w, float x) const;

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
};

}  // namespace gt::sim

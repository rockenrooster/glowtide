// Hand-made levels (GAME_DESIGN 7.2; VARIETY_DESIGN 10.1, 10.3): the smallest lane that shows a new thing.
#include "sim/level_build.h"

namespace gt::sim {

LevelDef introLevel(int index) {
    LevelDef L;
    L.index = index;
    L.seed = defaultSeed(index);
    L.world = 1;
    L.targetT = targetTime(index);
    L.fight = fightFor(index);
    L.hardness = hardnessFor(index);
    switch (index) {
        case 1:
            L.len = 30;
            addRow(L, 12, kPatHand);
            addGate(L, mulGate(0, 1.8f, 12, 2, 0));
            L.spireHealth = 66;  // wins in about 6 s (GAME_DESIGN 8: 6 to 10 s) with shots from the barrel tip
            break;
        case 2:
            L.len = 30;
            addRow(L, 10, kPatHand);
            addGate(L, mulGate(-2.5f, 1.8f, 10, 2, 0));
            addGate(L, mulGate(2.5f, 1.8f, 10, 3, 0));
            addEmitter(L, 0);
            break;
        case 3:
            L.len = 36;
            addRow(L, 9, kPatHand);
            addRow(L, 17, kPatHand);
            addGate(L, mulGate(0, 1.8f, 9, 2, 0));
            addGate(L, mulGate(0, 1.5f, 17, 3, 1));
            addEmitter(L, 2);
            break;
        case 4: {
            L.len = 36;
            addRow(L, 9, kPatHand);
            addRow(L, 17, kPatHand);
            addGate(L, mulGate(-2.7f, 1.5f, 9, 3, 0));
            addGate(L, divGate(2.7f, 1.5f, 9, 2, 0));
            Gate s = mulGate(0, 1.6f, 17, 2, 1);
            s.slides = 1;
            s.amp = 3.0f;
            s.period = 4.0f;
            addGate(L, s);
            addEmitter(L, 2);
            break;
        }
        case 6: {
            // The first geode: one plain LANTERN down the open side of a two-gate lane, while the enemy comes down the
            // gates' side (VARIETY_DESIGN 10.1). Hint: CRACK A GEODE TO GAIN ITS BOON.
            L.len = 32;
            L.world = 1;
            addRow(L, 9, kPatHand);
            addRow(L, 17, kPatHand);
            addGate(L, mulGate(-2.2f, 1.8f, 9, 2, 0));
            addGate(L, mulGate(-2.0f, 1.5f, 17, 3, 1));
            addEmitter(L, -1.6f);
            addBurst(L, 0.6f * L.targetT, 0, tune.burstMult, tune.burstSeconds, 0);
            Geode g;
            g.x = 2.6f;
            g.t = 0.15f * L.targetT;
            g.speed = 1.4f;
            g.boon = kBoonLantern;
            g.hpSeconds = 2.5f;
            L.geodes[L.geodeCount++] = g;
            break;
        }
        case 11: {
            // The first shield: a LANTERN behind a plate down the quiet side. The best line is on the right; the left
            // doubles three times, so the stream that reaches the geode is strong enough to break the plate early.
            // Hint: BREAK ITS SHIELD FIRST.
            L.len = 34;
            L.world = 2;
            addRow(L, 8, kPatHand);
            addRow(L, 15, kPatHand);
            addRow(L, 22, kPatHand);
            addGate(L, mulGate(-2.4f, 1.8f, 8, 2, 0));
            addGate(L, mulGate(2.4f, 1.5f, 8, 3, 0));
            addGate(L, mulGate(2.4f, 1.5f, 15, 3, 1));
            addGate(L, mulGate(-2.4f, 1.5f, 15, 2, 1));
            addGate(L, mulGate(2.2f, 1.3f, 22, 4, 2));
            addGate(L, mulGate(-2.4f, 1.4f, 22, 2, 2));
            addEmitter(L, -2.0f);
            addEmitter(L, 2.0f);
            addBurst(L, 0.4f * L.targetT, 0, tune.burstMult, tune.burstSeconds, 0);
            addBurst(L, 0.75f * L.targetT, 0, tune.burstMult, tune.burstSeconds, 0);
            Geode g;
            g.x = -2.6f;
            g.t = 0.07f * L.targetT;
            g.speed = 1.3f;
            g.boon = kBoonLantern;
            g.hpSeconds = 1.0f;
            g.shieldSeconds = 2.0f;
            L.geodes[L.geodeCount++] = g;
            break;
        }
        case 23: {
            // The first WARD: the enemy comes close down the middle. The right holds a x5 behind a wall but halves at the
            // top, so the best line is on the left; a BRIGHT comes down the right (a detour), then a WARD down the middle
            // when the line is at its lowest.
            L.len = 40;
            L.world = 3;
            addRow(L, 9, kPatHand);
            addRow(L, 16, kPatHand);
            addRow(L, 23, kPatHand);
            addRow(L, 30, kPatHand);
            addGate(L, mulGate(-2.6f, 1.6f, 9, 2, 0));
            addGate(L, mulGate(2.6f, 1.6f, 9, 3, 0));
            addGate(L, mulGate(2.6f, 1.5f, 18, 5, 1));
            addGate(L, mulGate(-2.6f, 1.5f, 16, 2, 1));
            L.barriers[L.barrierCount++] = {1.0f, 4.2f, 16.0f, 60};
            addGate(L, mulGate(0, 1.6f, 23, 3, 2));
            addGate(L, mulGate(-2.4f, 1.4f, 30, 4, 3));
            addGate(L, divGate(2.4f, 1.4f, 30, 2, 3));
            addEmitter(L, -1.5f);
            addEmitter(L, 0);
            addEmitter(L, 1.5f);
            addBurst(L, 0.35f * L.targetT, 0, tune.burstMult, tune.burstSeconds, 0);
            addBurst(L, 0.55f * L.targetT, 0, tune.burstMult, tune.burstSeconds, 0);
            L.fight = fightFor(index) + 0.05f;  // the enemy gets close here on purpose
            Geode b;
            b.x = 2.9f;  // a detour: the best line is on the left
            b.t = 0.12f * L.targetT;
            b.speed = 1.5f;
            b.boon = kBoonBright;
            b.hpSeconds = 2.5f;
            L.geodes[L.geodeCount++] = b;
            Geode w;
            w.x = 0;
            w.t = 0.5f * L.targetT;
            w.speed = 1.8f;
            w.boon = kBoonWard;
            w.hpSeconds = 1.7f;
            L.geodes[L.geodeCount++] = w;
            break;
        }
        default: {  // 5
            L.len = 34;
            addRow(L, 8, kPatHand);
            addRow(L, 15, kPatHand);
            addRow(L, 22, kPatHand);
            addGate(L, mulGate(0, 1.8f, 8, 2, 0));
            addGate(L, mulGate(-2.6f, 1.5f, 15, 2, 1));
            addGate(L, mulGate(2.6f, 1.5f, 15, 4, 1));
            L.barriers[L.barrierCount++] = {1.0f, 4.2f, 13.0f, 40};
            addGate(L, mulGate(0, 1.2f, 22, 2, 2));
            L.bumpers[L.bumperCount++] = {-3.1f, 22.0f, 0.7f};
            L.bumpers[L.bumperCount++] = {3.1f, 22.0f, 0.7f};
            addEmitter(L, -2);
            addEmitter(L, 2);
            addBurst(L, -1, 0.5f, tune.burstMult, tune.burstSeconds, 0);
            break;
        }
    }
    applyEnemyDefaults(L);
    return L;
}

}  // namespace gt::sim

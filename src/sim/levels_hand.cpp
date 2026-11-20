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

#include "lizard.h"

void cAccumulator::Clear() {
    for (int i = 0; i < 16; i++) {
        hidden[i] = 0;
        }
}

void cAccumulator::Add(int cl, int pc, int sq) {

    int idx = Idx(cl, pc, sq);

    for (int i = 0; i < 16; i++)
        hidden[i] += Network.quantized[i][idx];
}

void cAccumulator::Del(int cl, int pc, int sq) {

    int idx = Idx(cl, pc, sq);

    for (int i = 0; i < 16; i++)
        hidden[i] -= Network.quantized[i][idx];
}

void cAccumulator::SetFromScratch(Position* p) {

    Clear();

    for (int sq = 0; sq < 64; sq++)
    {
        if (p->IsFilled(sq))
            Accumulator.Add(ClOnSq(p, sq), TpOnSq(p, sq), sq);
    }
}
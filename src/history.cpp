#include "lizard.h"

void ClearHist(void) {

    for (int i = 0; i < 12; i++)
        for (int j = 0; j < 64; j++) {
            history[i][j] = 0;
            triedHistory[i][j] = 0;
        }

    for (int i = 0; i < MAX_PLY; i++) {
        killer[i][0] = 0;
        killer[i][1] = 0;
    }
}

void TrimHistory()
{
    for (int i = 0; i < 12; i++)
        for (int j = 0; j < 64; j++) {
            history[i][j] /= 2;
            triedHistory[i][j] /= 2;
        }

}

void UpdateTried(Position* p, int move, int depth)
{
    // Don't update stuff used for move ordering if a move changes material balance

    if (p->pc[Tsq(move)] != NO_PC || IsProm(move) || MoveType(move) == EP_CAP)
        return;

    // Increment history counter

    triedHistory[p->pc[Fsq(move)]][Tsq(move)] += depth * depth;

    // Prevent history counters from growing too high

    if (triedHistory[p->pc[Fsq(move)]][Tsq(move)] > (1 << 15)) {
        TrimHistory();
    }
}

void UpdateHistory(Position* p, int move, int depth, int ply) {

    // Don't update stuff used for move ordering if a move changes material balance

    if (p->pc[Tsq(move)] != NO_PC || IsProm(move) || MoveType(move) == EP_CAP)
        return;

    // Increment history counter

    history[p->pc[Fsq(move)]][Tsq(move)] += depth * depth;

    // Prevent history counters from growing too high

    if (history[p->pc[Fsq(move)]][Tsq(move)] > (1 << 15)) {
        TrimHistory();
    }

    // Update killer moves, taking care that they are different

    if (move != killer[ply][0]) {
        killer[ply][1] = killer[ply][0];
        killer[ply][0] = move;
    }
}

int GetHistory(int pc, int sq) {
    return 1000 * history[pc][sq] / (triedHistory[pc][sq] + 1);
}
#include "lizard.h"

int Quiesce(Position* p, int ply, int alpha, int beta, int* pv) {

    int best, score, move, new_pv[MAX_PLY];
    MOVES m[1];
    UNDO u[1];

    // Statistics and attempt at quick exit

    nodes++;
#ifdef USE_TUNING
    abort_search = 0;
#else
    CheckForTimeout();
    if (abort_search) return 0;
#endif
    *pv = 0;
    if (IsDraw(p)) return 0;
    if (ply >= MAX_PLY - 1) return Evaluate(p, 1);

    // Get a stand-pat score and adjust bounds
    // (exiting if eval exceeds beta)

    best = Evaluate(p, 1);
    if (best >= beta) return best;
    if (best > alpha) alpha = best;

    // Transposition table read
#ifndef USE_TUNING
    if (TransRetrieve(p->hash_key, &move, &score, alpha, beta, 0, ply))
        return score;
#endif
    InitCaptures(p, m);

    // Main loop

    while ((move = NextCapture(m))) {

        // Delta pruning

        if (best + tp_value[TpOnSq(p, Tsq(move))] + 300 < alpha) continue;

        // Pruning of bad captures

        if (BadCapture(p, move)) continue;

        p->DoMove(move, u);
        if (Illegal(p)) { p->UndoMove(move, u); continue; }
        score = -Quiesce(p, ply + 1, -beta, -alpha, new_pv);
        p->UndoMove(move, u);
        if (abort_search) return 0;

        // Beta cutoff

        if (score >= beta) {
#ifndef USE_TUNING
            TransStore(p->hash_key, *pv, best, LOWER, 0, ply);
#endif
            return score;
        }

        // Adjust alpha and score

        if (score > best) {
            best = score;
            if (score > alpha) {
                alpha = score;
                BuildPv(pv, new_pv, move);
            }
        }
    }
#ifndef USE_TUNING
    if (*pv) TransStore(p->hash_key, *pv, best, EXACT, 0, ply);
    else 	   TransStore(p->hash_key, 0, best, UPPER, 0, ply);
#endif
    return best;
}

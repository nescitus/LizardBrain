#include "lizard.h"
#include "timer.h"
#include <stdio.h>

void Think(Position* p, int* pv) {

    ClearHist();
    tt_date = (tt_date + 1) & 255;
    nodes = 0;
    abort_search = 0;
    Timer.SetStartTime();
    Iterate(p, pv);
}

void Iterate(Position* p, int* pv) {

    int val = 0, cur_val = 0;
    U64 nps = 0;
    Timer.SetIterationTiming();

    // TODO: use Perft() to reduce max depth if only one move is available

    for (root_depth = 1; root_depth <= Timer.GetData(MAX_DEPTH); root_depth++) {
        int elapsed = Timer.GetElapsedTime();
        if (elapsed) nps = nodes * 1000 / elapsed;
        printf("info depth %d time %d nodes %I64d nps %I64d\n", root_depth, elapsed, nodes, nps);
        cur_val = Widen(p, root_depth, pv, cur_val);
        if (abort_search || Timer.FinishIteration()) break;
        val = cur_val;
    }
}

int Widen(Position* p, int depth, int* pv, int lastScore) {

    // Function performs aspiration search, progressively widening the window.
    // Code structere modelled after Senpai 1.0.

    int cur_val = lastScore, alpha, beta;

    if (depth > 6 && lastScore < MAX_EVAL) {
        for (int margin = 10; margin < 500; margin *= 2) {
            alpha = lastScore - margin;
            beta = lastScore + margin;
            cur_val = Search(p, 0, alpha, beta, depth, 0, pv);
            if (abort_search) break;
            if (cur_val > alpha && cur_val < beta)
                return cur_val;            // we have finished within the window
            if (cur_val > MAX_EVAL) break; // verify mate searching with infinite bounds
        }
    }

    cur_val = Search(p, 0, -INF, INF, root_depth, 0, pv);      // full window search
    return cur_val;
}

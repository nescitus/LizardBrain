#include "lizard.h"
#include "math.h"

void InitLmr(void) {

    // Set depth of late move reduction using modified Stockfish formula

    for (int depth = 0; depth < MAX_PLY; depth++)
        for (int moves = 0; moves < MAX_MOVES; moves++) {
            lmrSize[0][depth][moves] = (0.33 + log((double)(depth)) * log((double)(moves)) / 2.25); // zw node
            lmrSize[1][depth][moves] = (0.00 + log((double)(depth)) * log((double)(moves)) / 3.50); // pv node

            for (int node = 0; node <= 1; node++) {
                if (lmrSize[node][depth][moves] < 1) lmrSize[node][depth][moves] = 0; // ultra-small reductions make no sense
                else lmrSize[node][depth][moves] += 0.5;

                if (lmrSize[node][depth][moves] > depth - 1) // reduction cannot exceed actual depth
                    lmrSize[node][depth][moves] = depth - 1;
            }
        }
}

#include <assert.h>
#include <stdio.h>
#include "lizard.h"

float nnValue;
const int hiddenLayerSize = 16;

int Evaluate(Position* p) {

    int score = EvalNN(p);

    score += EvalPieces(p, White);
    score -= EvalPieces(p, Black);
    
    // Scale down drawish endgames

    int draw_factor = 64;
    if (score > 0) draw_factor = GetDrawFactor(p, White);
    else           draw_factor = GetDrawFactor(p, Black);
    score *= draw_factor;
    score /= 64;

    // Make sure eval doesn't exceed mate score

    if (score < -MAX_EVAL)
        score = -MAX_EVAL;
    else if (score > MAX_EVAL)
        score = MAX_EVAL;

    // Return score relative to the side to move

    return p->side == White ? score : -score;
}

int EvalNN(Position* p) {

    // Neural network chess evaluation

    nnValue = 0; // external
    float hidden[hiddenLayerSize];
    float output[hiddenLayerSize];

    for (int i = 0; i < hiddenLayerSize; i++) {
        hidden[i] = 0;
        output[i] = 0;
    }

    // calculation for the hidden layer

    for (int i = 0; i < hiddenLayerSize; i++)
    {
        hidden[i] = Accumulator.hidden[i] * Network.hiddenWeights[i];
        if (hidden[i] > 0) // ReLu condition
            output[i] += hidden[i] * Network.outputWeights[i];
    }

    // get final output

    for (int i = 0; i < hiddenLayerSize; i++)
        nnValue += output[i];
    
    // final multiplication to keep result within proper scale

    nnValue *= Network.finalWeight;

    return (int)nnValue;
}

int Idx(int x, int y, int z) {
    return 64 * (6 * x + y) + z;
}

int EvalPieces(Position* p, int side) {

    U64 bbPieces, bbMob;
    int sq, cnt;
    int result = 0;

    bbPieces = p->Map(side, Knight);
    while (bbPieces) {
        sq = PopFirstBit(&bbPieces);

        // Knight mobility

        bbMob = n_attacks[sq] & ~p->cl_bb[side];
        cnt = PopCnt(bbMob) - 4;
        result += 4 * cnt;
    }

    bbPieces = p->Map(side, Bishop);
    while (bbPieces) {
        sq = PopFirstBit(&bbPieces);

        // Bishop mobility

        bbMob = BAttacks(OccBb(p), sq);
        cnt = PopCnt(bbMob) - 7;
        result += 5 * cnt;
    }

    bbPieces = p->Map(side, Rook);
    while (bbPieces) {
        sq = PopFirstBit(&bbPieces);

        // Rook mobility

        bbMob = RAttacks(OccBb(p), sq);
        cnt = PopCnt(bbMob) - 7;
        result += 2 * cnt;
    }

    bbPieces = p->Map(side, Queen);
    while (bbPieces) {
        sq = PopFirstBit(&bbPieces);

        // Queen mobility

        bbMob = BAttacks(OccBb(p), sq);
        cnt = PopCnt(bbMob) - 14;
        result += 1 * cnt;
    }

    return result;
}

#include <assert.h>
#include <stdio.h>
#include "lizard.h"

float nnValue;
const int hiddenLayerSize = 16;
sEvalHashEntry EvalTT[EVAL_HASH_SIZE];

int kingRoot[64] = {
 B2,  B2,  C2,  D2,  E2,  F2,  G2,  G2,
 B2,  B2,  C2,  D2,  E2,  F2,  G2,  G2,
 B3,  B3,  C3,  D3,  E3,  F3,  G3,  G3,
 B4,  B4,  C4,  D4,  E4,  F4,  G4,  G4,
 B5,  B5,  C5,  D5,  E5,  F5,  G5,  G5,
 B6,  B6,  C6,  D6,  E6,  F6,  G6,  G6,
 B7,  B7,  C7,  D7,  E7,  F7,  G7,  G7,
 B7,  B7,  C7,  D7,  E7,  F7,  G7,  G7,
};

static const int SafetyTable[100] = {
    0,  0,   1,   2,   3,   5,   7,   9,  12,  15,
  18,  22,  26,  30,  35,  39,  44,  50,  56,  62,
  68,  75,  82,  85,  89,  97, 105, 113, 122, 131,
 140, 150, 169, 180, 191, 202, 213, 225, 237, 248,
 260, 272, 283, 295, 307, 319, 330, 342, 354, 366,
 377, 389, 401, 412, 424, 436, 448, 459, 471, 483,
 494, 500, 500, 500, 500, 500, 500, 500, 500, 500,
 500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
 500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
 500, 500, 500, 500, 500, 500, 500, 500, 500, 500
};

int Evaluate(Position* p) {

    // Try to retrieve score from eval hashtable
#ifndef USE_TUNING
    int addr = p->hash_key % EVAL_HASH_SIZE;

    if (EvalTT[addr].key == p->hash_key) {
        int hashScore = EvalTT[addr].score;
        return p->side == White ? hashScore : -hashScore;
    }
#endif

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

    // Save eval score in the evaluation hash table
#ifndef USE_TUNING
    EvalTT[addr].key = p->hash_key;
    EvalTT[addr].score = score;
#endif

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
        float dequantized = (float)Accumulator.hidden[i] / scaleFactor;
        hidden[i] = dequantized * Network.hiddenWeights[i];
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

    U64 bbPieces, bbAtt, bbMob;
    int sq, cnt;
    int result = 0;
    int op = Opp(side);
    int ksq = KingSq(p, op);

    // Init enemy king zone for attack evaluation

    int att = 0;
    U64 bbZone = k_attacks[kingRoot[ksq]];

    bbPieces = p->Map(side, Knight);
    while (bbPieces) {
        sq = PopFirstBit(&bbPieces);

        // Knight mobility

        bbMob = n_attacks[sq] & ~p->cl_bb[side];
        cnt = PopCnt(bbMob) - 4;
        result += 4 * cnt;

        // Knight attacks on enemy king zone

        bbAtt = n_attacks[sq];
        if (bbAtt & bbZone) {
            att += 2 * PopCnt(bbAtt & bbZone);
        }
    }

    bbPieces = p->Map(side, Bishop);
    while (bbPieces) {
        sq = PopFirstBit(&bbPieces);

        // Bishop mobility

        bbMob = BAttacks(OccBb(p), sq);
        cnt = PopCnt(bbMob) - 7;
        result += 5 * cnt;

        // Bishop attacks on enemy king zone

        bbAtt = BAttacks(OccBb(p) ^ p->Map(side, Queen), sq);
        bbAtt &= bbZone;
        if (bbAtt) {
            att += 2 * PopCnt(bbAtt);
        }
    }

    bbPieces = p->Map(side, Rook);
    while (bbPieces) {
        sq = PopFirstBit(&bbPieces);

        // Rook mobility

        bbMob = RAttacks(OccBb(p), sq);
        cnt = PopCnt(bbMob) - 7;
        result += 2 * cnt;

        // Rook attacks on enemy king zone

        bbAtt = RAttacks(OccBb(p) ^ p->Map(side, Queen) ^ p->Map(side, Rook), sq);
        bbAtt &= bbZone;
        if (bbAtt) {
            att += 3 * PopCnt(bbAtt);
        }
    }

    bbPieces = p->Map(side, Queen);
    while (bbPieces) {
        sq = PopFirstBit(&bbPieces);

        // Queen mobility

        bbMob = BAttacks(OccBb(p), sq);
        cnt = PopCnt(bbMob) - 14;
        result += 1 * cnt;

        // Queen attacks on enemy king zone

        bbAtt = BAttacks(OccBb(p) ^ p->Map(side, Bishop) ^ p->Map(side, Queen), sq);
        bbAtt |= RAttacks(OccBb(p) ^ p->Map(side, Rook) ^ p->Map(side, Queen), sq);
        bbAtt &= bbZone;
        if (bbAtt) {
            att += 5 * PopCnt(bbAtt);
        }
    }

    result += SafetyTable[Min(att, 99)];

    return result;
}

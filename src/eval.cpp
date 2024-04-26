#include <assert.h>
#include <stdio.h>
#include "lizard.h"
#include "eval.h"

sEvalHashEntry EvalTT[EVAL_HASH_SIZE];
double nnValue;
const int hiddenLayerSize = 16;

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

static const int maxPhase = 24;
const int phase_value[7] = { 0, 1, 1, 2, 4, 0, 0 };

static const U64 bbQSCastle[2] = { SqBb(A1) | SqBb(B1) | SqBb(C1) | SqBb(A2) | SqBb(B2) | SqBb(C2),
                                   SqBb(A8) | SqBb(B8) | SqBb(C8) | SqBb(A7) | SqBb(B7) | SqBb(C7)
};
static const U64 bbKSCastle[2] = { SqBb(F1) | SqBb(G1) | SqBb(H1) | SqBb(F2) | SqBb(G2) | SqBb(H2),
                                   SqBb(F8) | SqBb(G8) | SqBb(H8) | SqBb(F7) | SqBb(G7) | SqBb(H7)
};

static const int bStartKs[2] = { F1, F8 };
static const int bStartQs[2] = { C1, C8 };
static const int pStartKs[2] = { E2, E7 };
static const int pStartQs[2] = { D2, D7 };
static const int pBlockKs[2] = { E3, E6 };
static const int pBlockQs[2] = { D3, D6 };

static const U64 bbCentralFile = FILE_C_BB | FILE_D_BB | FILE_E_BB | FILE_F_BB;

static const U64 kingMaskKs[2] = { SqBb(F1) | SqBb(G1), SqBb(F8) | SqBb(G8) };
static const U64 kingMaskQs[2] = { SqBb(C1) | SqBb(D1), SqBb(C8) | SqBb(D8) };
static const U64 rookMaskKs[2] = { SqBb(G1) | SqBb(H1) | SqBb(H2), SqBb(G8) | SqBb(H8) | SqBb(H7) };
static const U64 rookMaskQs[2] = { SqBb(B1) | SqBb(A1) | SqBb(A2), SqBb(B8) | SqBb(A8) | SqBb(A7) };

bool isClosed;
U64 control[2];
U64 bbPawnTakes[2];
U64 bbPawnCanTake[2];
U64 support_mask[2][64];
int mg_pst_data[2][6][64];
int eg_pst_data[2][6][64];

void ClearEvalHash(void) {

    for (int e = 0; e < EVAL_HASH_SIZE; e++) {
        EvalTT[e].key = 0;
        EvalTT[e].score = 0;
    }
}

int Evaluate(Position* p, int use_hash) {

    // Try to retrieve score from eval hashtable

    int addr = p->hash_key % EVAL_HASH_SIZE;

    if (EvalTT[addr].key == p->hash_key && use_hash) {
        int hashScore = EvalTT[addr].score;
        return p->side == White ? hashScore : -hashScore;
    }

    int score = EvalNN(p);
    
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

    EvalTT[addr].key = p->hash_key;
    EvalTT[addr].score = score;

    // Return score relative to the side to move

    return p->side == White ? score : -score;
}

int EvalNN(Position* p) 
{
    // Neural network chess evaluation

    nnValue = 0; // external
    double hidden[hiddenLayerSize];
    double output[hiddenLayerSize];
    for (int i = 0; i < hiddenLayerSize; i++) {
        hidden[i] = 0;
        output[i] = 0;
    }

    // accumulate scores
    // (can be optimized by doing it incrementally
    // while making/unmaking moves)

    for (int sq = 0; sq < 64; sq++) {

        if (p->IsFilled(sq)) {
            int pieceColor = ClOnSq(p, sq);
            int pieceType = TpOnSq(p, sq);
            int idx = Idx(pieceColor, pieceType, sq);

            for (int i = 0; i < hiddenLayerSize; i++)
                hidden[i] += Network.weights[i][idx];
        }
    }

    // calculation for the next layer

    for (int i = 0; i < hiddenLayerSize; i++)
    {
        hidden[i] *= Network.hiddenWeights[i];
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
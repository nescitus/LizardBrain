#include <assert.h>
#include <stdio.h>
#include "lizard.h"
#include "eval.h"

float nnValue;
sEvalHashEntry EvalTT[EVAL_HASH_SIZE];

int mg[2];
int eg[2];
int phase;
U64 bbPawnTakes[2];
U64 control[2];

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

int Evaluate(Position* p) {

    // Try to retrieve score from eval hashtable
#ifndef USE_TUNING
    int addr = p->hash_key % EVAL_HASH_SIZE;

    if (EvalTT[addr].key == p->hash_key) {
        int hashScore = EvalTT[addr].score;
        return p->side == White ? hashScore : -hashScore;
    }
#endif

    mg[White] = mg[Black] = 0;
    eg[White] = eg[Black] = 0;
    phase = 0;

    int score = EvalNN(p);    
    
    bbPawnTakes[White] = GetWPControl(PcBb(p, White, Pawn));
    bbPawnTakes[Black] = GetBPControl(PcBb(p, Black, Pawn));
    control[White] = bbPawnTakes[White];
    control[Black] = bbPawnTakes[Black];

    EvalPieces(p, White);
    EvalPieces(p, Black);
    EvalPawns(p, White);
    EvalPawns(p, Black);
    score += EvalPressure(p, White);
    score -= EvalPressure(p, Black);

    int mgPhase = Min(phase, 24); // TODO: test 24
    int egPhase = 24 - mgPhase;
    int mgScore = mg[White] - mg[Black];
    int egScore = eg[White] - eg[Black];

    score += ((mgPhase * mgScore) + (egPhase * egScore)) / 24;
    
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

void EvalPieces(Position* p, int side) {

    U64 bbPieces, bbAtt, bbMob, bbSafe;
    int sq, cnt;
    int result = 0;
    int op = Opp(side);
    int ksq = KingSq(p, op);
    control[op] |= k_attacks[ksq];

    U64 nChecks = n_attacks[ksq];
    U64 bChecks = BAttacks(OccBb(p), ksq);
    U64 rChecks = RAttacks(OccBb(p), ksq);
    U64 qChecks = rChecks | bChecks;

    // Init enemy king zone for attack evaluation

    int att = 0;
    U64 bbZone = k_attacks[kingRoot[ksq]];

    bbPieces = p->Map(side, Knight);
    while (bbPieces) {
        sq = PopFirstBit(&bbPieces);
        phase += 1;

        // Knight mobility

        bbMob = n_attacks[sq] & ~p->cl_bb[side];
        bbSafe = bbMob & ~bbPawnTakes[op];
        cnt = PopCnt(bbMob) - 4;
        mg[side] += 4 * cnt;
        eg[side] += 4 * cnt;

        // Knight check threats

        if (bbSafe & nChecks)
            att += 14 * PopCnt(bbSafe & nChecks);

        // Knight attacks on enemy king zone

        bbAtt = n_attacks[sq];
        control[side] |= bbAtt;
        if (bbAtt & bbZone) {
            att += 7 * PopCnt(bbAtt & bbZone);
        }
    }

    bbPieces = p->Map(side, Bishop);
    while (bbPieces) {
        sq = PopFirstBit(&bbPieces);
        phase += 1;

        // Bishop mobility

        bbMob = BAttacks(OccBb(p), sq);
        bbSafe = bbMob & ~bbPawnTakes[op];
        control[side] |= bbMob;
        cnt = PopCnt(bbMob) - 7;
        mg[side] += 5 * cnt;
        eg[side] += 5 * cnt;

        // Bishop check threats

        if (bbSafe & bChecks)
            att += 20 * PopCnt(bbSafe & bChecks);

        // Bishop attacks on enemy king zone

        bbAtt = BAttacks(OccBb(p) ^ p->Map(side, Queen), sq);
        bbAtt &= bbZone;
        if (bbAtt) {
            att += 8 * PopCnt(bbAtt & ~bbPawnTakes[op]);
            att += 4 * PopCnt(bbAtt & bbPawnTakes[op]);
        }
    }

    bbPieces = p->Map(side, Rook);
    while (bbPieces) {
        sq = PopFirstBit(&bbPieces);
        phase += 2;

        // Rook mobility

        bbMob = RAttacks(OccBb(p), sq);
        bbSafe = bbMob & ~bbPawnTakes[op];
        control[side] |= bbMob;
        cnt = PopCnt(bbMob) - 7;
        mg[side] += 2 * cnt;
        eg[side] += 4 * cnt;

        // Rook check threats

        if (bbSafe & rChecks)
            att += 18 * PopCnt(bbSafe & rChecks);

        // Rook attacks on enemy king zone

        bbAtt = RAttacks(OccBb(p) ^ p->Map(side, Queen) ^ p->Map(side, Rook), sq);
        bbAtt &= bbZone;
        if (bbAtt) {
            att += 12 * PopCnt(bbAtt & ~bbPawnTakes[op]);
            att +=  5 * PopCnt(bbAtt & bbPawnTakes[op]);
        }

        // Rook on (half) open file

        U64 bbFile = FillNorth(SqBb(sq)) | FillSouth(SqBb(sq));
        if ((bbFile & p->Map(side, Pawn))) {
            mg[side] -= 6;
            eg[side] -= 3;
        }
        else {
            if (!(bbFile & p->Map(op, Pawn))) {
                mg[side] += 6;
                eg[side] += 6;
            }
            else {
                mg[side] += 3;
                eg[side] += 3;
            }
        }
    }

    bbPieces = p->Map(side, Queen);
    while (bbPieces) {
        sq = PopFirstBit(&bbPieces);
        phase += 4;

        // Queen mobility

        bbMob = QAttacks(OccBb(p), sq);
        control[side] |= bbMob;
        cnt = PopCnt(bbMob) - 14;
        mg[side] += 1 * cnt;
        eg[side] += 2 * cnt;

        // Queen check threats

        if (bbMob & qChecks)
            att += 12 * PopCnt(bbMob & qChecks);

        // Queen attacks on enemy king zone

        bbAtt = BAttacks(OccBb(p) ^ p->Map(side, Bishop) ^ p->Map(side, Queen), sq);
        bbAtt |= RAttacks(OccBb(p) ^ p->Map(side, Rook) ^ p->Map(side, Queen), sq);
        bbAtt &= bbZone;
        if (bbAtt) {
            att += 15 * PopCnt(bbAtt & ~bbPawnTakes[op]);
            att +=  5 * PopCnt(bbAtt & bbPawnTakes[op]);
        }
    }

    mg[side] += Danger.tab[Min(att, 500)];
}

void EvalPawns(Position* p, int side) {

    int sq;
    U64 bbPieces = p->Map(side, Pawn);

    while (bbPieces) {
        sq = PopFirstBit(&bbPieces);

        if ((passed_mask[side][sq] & PcBb(p, Opp(side), Pawn)) == 0)
        {
            mg[side] += passed_bonus_mg[side][Rank(sq)];
            eg[side] += passed_bonus_eg[side][Rank(sq)];
        }
    }
}

int EvalPressure(Position* p, int sd) {
    int result = 0;
    int s, t;
    U64 opp = p->cl_bb[Opp(sd)];
    U64 att = control[sd];
    U64 def = control[Opp(sd)];
    U64 ctrl = att & ~def;
    U64 hang = (opp & ctrl) | (opp & bbPawnTakes[sd]);

    // enemy pieces, hanging and attacked

    while (hang) {
        s = PopFirstBit(&hang);
        t = Tp(p->pc[s]);
        result += hanging[t];
    }

    return result;
}
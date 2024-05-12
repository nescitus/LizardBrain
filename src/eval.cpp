#include <assert.h>
#include <stdio.h>
#include "lizard.h"

float nnValue;
const int hiddenLayerSize = 16;

int Evaluate(Position* p) {

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
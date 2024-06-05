#include "lizard.h"

#define USE_AVX

#ifdef USE_AVX
#include <immintrin.h>
#endif

void cAccumulator::Clear() {
    for (int i = 0; i < hiddenLayerSize; i++)
        hidden[i] = 0;
}

#ifdef USE_AVX

void cAccumulator::Add(int cl, int pc, int sq) {

    // get piece-on-square index
    int idx = Idx(cl, pc, sq);

    // process 8 elements at a time using AVX2 (256-bit)
    for (int i = 0; i < hiddenLayerSize; i += 8) {

        // load 8 integers from the hidden and quantized arrays
        __m256i hidden_vals = _mm256_load_si256((__m256i*) & hidden[i]);
        __m256i quantized_vals = _mm256_load_si256((__m256i*) & Network.flat_quantized[idx * hiddenLayerSize + i]);

        // add the hidden and quantized values together
        __m256i result = _mm256_add_epi32(hidden_vals, quantized_vals);

        // store the result back into the hidden array
        _mm256_store_si256((__m256i*) & hidden[i], result);
    }
}

void cAccumulator::Del(int cl, int pc, int sq) {

    // get piece-on-square index
    int idx = Idx(cl, pc, sq);

    // process 8 elements at a time using AVX2 (256-bit)
    for (int i = 0; i < hiddenLayerSize; i += 8) {

        // load 8 integers from the hidden and quantized arrays
        __m256i hidden_vals = _mm256_load_si256((__m256i*) & hidden[i]);
        __m256i quantized_vals = _mm256_load_si256((__m256i*) & Network.flat_quantized[idx * hiddenLayerSize + i]);

        // add the hidden and quantized values together
        __m256i result = _mm256_sub_epi32(hidden_vals, quantized_vals);

        // store the result back into the hidden array
        _mm256_store_si256((__m256i*) & hidden[i], result);
    }
}

void cAccumulator::Move(int cl, int pc, int fsq, int tsq) {
    int idx_fr = Idx(cl, pc, fsq);
    int idx_to = Idx(cl, pc, tsq);

    // Process 8 elements at a time using AVX2 (256-bit)
    for (int i = 0; i < hiddenLayerSize; i += 8) {
        // Load 8 integers from the hidden and quantized arrays
        __m256i hidden_vals = _mm256_load_si256((__m256i*) & hidden[i]);

        // Load 8 integers from the quantized array for the given indices
        __m256i quantized_from_vals = _mm256_load_si256((__m256i*) & Network.flat_quantized[idx_fr * hiddenLayerSize + i]);
        __m256i quantized_to_vals   = _mm256_load_si256((__m256i*) & Network.flat_quantized[idx_to * hiddenLayerSize + i]);

        // Add the quantized_to_vals and subtract the quantized_from_vals from hidden_vals
        __m256i result = _mm256_add_epi32(hidden_vals, quantized_to_vals);
        result = _mm256_sub_epi32(result, quantized_from_vals);

        // Store the result back into the hidden array
        _mm256_store_si256((__m256i*) & hidden[i], result);
    }
}

#else

void cAccumulator::Add(int cl, int pc, int sq) {

    int idx = Idx(cl, pc, sq);

    for (int i = 0; i < hiddenLayerSize; i++)
        hidden[i] += Network.quantized[i][idx];
}

void cAccumulator::Del(int cl, int pc, int sq) {

    int idx = Idx(cl, pc, sq);

    for (int i = 0; i < hiddenLayerSize; i++)
        hidden[i] -= Network.quantized[i][idx];
}

void cAccumulator::Move(int cl, int pc, int fsq, int tsq) {
    Add(cl, pc, tsq);
    Del(cl, pc, fsq);
}

#endif

void cAccumulator::SetFromScratch(Position* p) {

    Clear();

    for (int sq = 0; sq < 64; sq++)
    {
        if (p->IsFilled(sq))
            Accumulator.Add(ClOnSq(p, sq), TpOnSq(p, sq), sq);
    }
}
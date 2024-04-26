#include "lizard.h"

#define FAST_POPCNT

int RelSq(int c, int s) {
    return (s ^ (c * 56));
}
#ifdef FAST_POPCNT

#include <nmmintrin.h>
#include <intrin.h>

int PopCnt(U64 bb) {
    return (int)_mm_popcnt_u64(bb);
}

int PopFirstBit(U64* bb) {

    U64 bbLocal = *bb;
    *bb &= (*bb - 1);

    return FirstOne(bbLocal);
}


#else

int PopCnt(U64 bb) {

  U64 k1 = (U64)0x5555555555555555;
  U64 k2 = (U64)0x3333333333333333;
  U64 k3 = (U64)0x0F0F0F0F0F0F0F0F;
  U64 k4 = (U64)0x0101010101010101;

  bb -= (bb >> 1) & k1;
  bb = (bb & k2) + ((bb >> 2) & k2);
  bb = (bb + (bb >> 4)) & k3;
  return (bb * k4) >> 56;
}

int PopFirstBit(U64* bb) {

    U64 bbLocal = *bb;
    *bb &= (*bb - 1);
    return FirstOne(bbLocal);
}

#endif
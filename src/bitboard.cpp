#include "lizard.h"

#define FAST_POPCNT

int RelSq(int c, int s) {
    return (s ^ (c * 56));
}
#ifdef FAST_POPCNT

#include <nmmintrin.h>
#include <intrin.h>

int PopCnt(U64 b) {
    return (int)_mm_popcnt_u64(b);
}

int PopFirstBit(U64* b) {

    U64 bbLocal = *b;
    *b &= (*b - 1);

    return FirstOne(bbLocal);
}


#else

int PopCnt(U64 b) {

    U64 k1 = (U64)0x5555555555555555;
    U64 k2 = (U64)0x3333333333333333;
    U64 k3 = (U64)0x0F0F0F0F0F0F0F0F;
    U64 k4 = (U64)0x0101010101010101;

    b -= (b >> 1) & k1;
    b = (b & k2) + ((b >> 2) & k2);
    b = (b + (b >> 4)) & k3;
    return (b * k4) >> 56;
}

int PopFirstBit(U64* b) {

    U64 bbLocal = *b;
    *b &= (*b - 1);
    return FirstOne(bbLocal);
}

#endif

U64 FillNorth(U64 b) {
    b |= b << 8;
    b |= b << 16;
    b |= b << 32;
    return b;
}

U64 FillSouth(U64 b) {
    b |= b >> 8;
    b |= b >> 16;
    b |= b >> 32;
    return b;
}

U64 GetWPControl(U64 b) {
    return (ShiftNE(b) | ShiftNW(b));
}

U64 GetBPControl(U64 b) {
    return (ShiftSE(b) | ShiftSW(b));
}

U64 GetFrontSpan(U64 b, int sd) {

    if (sd == White) return FillNorth(ShiftNorth(b));
    else             return FillSouth(ShiftSouth(b));
}

U64 GetFwd(U64 b, int sd) {

    if (sd == White) return ShiftNorth(b);
    else          return ShiftSouth(b);
}
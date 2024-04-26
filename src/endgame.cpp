#include "lizard.h"

int GetDrawFactor(Position* p, int strong) {

    int weak = Opp(strong);

    if (p->cnt[strong][Pawn] == 0) {

        if (p->MajorCnt(White) + p->MajorCnt(Black) == 0) {

            // K(m) vs K(m) or Km vs Kp(p)
            if (p->MinorCnt(strong) < 2) return 0;

            if (p->MinorCnt(strong) == 1 && p->MinorCnt(weak) == 1)
                return 8;

            if (p->MinorCnt(strong) == 2 && p->MinorCnt(weak) == 2)
                return 16;

            if (p->MinorCnt(strong) == 2 && p->MinorCnt(weak) == 1) {
                if (p->cnt[strong][Bishop] < 2 || p->cnt[weak][Bishop] > 0)
                    return 32;
            }

            if (p->cnt[strong][Knight] == 2 && p->cnt[strong][Bishop] == 0
                && p->cnt[weak][Pawn] == 0)
                return 0;


        }

        // KR vs Km(p)
        if (p->cnt[strong][Queen] + p->MinorCnt(strong) == 0 && p->cnt[strong][Rook] == 1
            && p->cnt[weak][Queen] + p->cnt[weak][Rook] == 0 && p->MinorCnt(weak) == 1) return 32; // 1/2

            // KRm vs KR(p)
        if (p->cnt[strong][Queen] == 0 && p->MinorCnt(strong) == 1 && p->cnt[strong][Rook] == 1
            && p->cnt[weak][Queen] + p->MinorCnt(weak) == 0 && p->cnt[weak][Rook] == 1) return 32; // 1/2
    }

    return 64; // default: no scaling
}

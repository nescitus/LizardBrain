#include "lizard.h"

U64 AttacksFrom(Position *p, int sq) {

  switch (TpOnSq(p, sq)) {
  case Pawn:
    return p_attacks[Cl(p->pc[sq])][sq];
  case Knight:
    return n_attacks[sq];
  case Bishop:
    return BAttacks(OccBb(p), sq);
  case Rook:
    return RAttacks(OccBb(p), sq);
  case Queen:
    return QAttacks(OccBb(p), sq);
  case King:
    return k_attacks[sq];
  }
  return 0;
}

U64 AttacksTo(Position *p, int sq) {

  return (PcBb(p, White, Pawn) & p_attacks[Black][sq]) |
         (PcBb(p, Black, Pawn) & p_attacks[White][sq]) |
         (p->tp_bb[Knight] & n_attacks[sq]) |
         ((p->tp_bb[Bishop] | p->tp_bb[Queen]) & BAttacks(OccBb(p), sq)) |
         ((p->tp_bb[Rook] | p->tp_bb[Queen]) & RAttacks(OccBb(p), sq)) |
         (p->tp_bb[King] & k_attacks[sq]);
}

int Attacked(Position *p, int sq, int side) {

  return (PcBb(p, side, Pawn) & p_attacks[Opp(side)][sq]) ||
         (PcBb(p, side, Knight) & n_attacks[sq]) ||
         ((PcBb(p, side, Bishop) | PcBb(p, side, Queen)) & BAttacks(OccBb(p), sq)) ||
         ((PcBb(p, side, Rook) | PcBb(p, side, Queen)) & RAttacks(OccBb(p), sq)) ||
         (PcBb(p, side, King) & k_attacks[sq]);
}

#include "lizard.h"

int *GenerateCaptures(Position *p, int *list) {

  U64 bbPieces, bbMoves, bbEnemy;
  int from, to;
  int side = p->side;

  bbEnemy = p->cl_bb[Opp(side)];

  if (side == White) {

    // White pawn promotions with capture

    bbMoves = ((p->Map(White, Pawn) & ~FILE_A_BB & RANK_7_BB) << 7) & p->cl_bb[Black];
    while (bbMoves) {
      to = PopFirstBit(&bbMoves);
      list = SerializePromotions(list, to - 7, to);
    }

  // White pawn promotions with capture

    bbMoves = ((PcBb(p, White, Pawn) & ~FILE_H_BB & RANK_7_BB) << 9) & p->cl_bb[Black];
    while (bbMoves) {
      to = PopFirstBit(&bbMoves);
      list = SerializePromotions(list, to - 9, to);
    }

  // White pawn promotions without capture

    bbMoves = ((PcBb(p, White, Pawn) & RANK_7_BB) << 8) & UnoccBb(p);
    while (bbMoves) {
      to = PopFirstBit(&bbMoves);
      list = SerializePromotions(list, to - 8, to);
    }

  // White pawn captures

    bbMoves = ((PcBb(p, White, Pawn) & ~FILE_A_BB & ~RANK_7_BB) << 7) & p->cl_bb[Black];
    while (bbMoves) {
      to = PopFirstBit(&bbMoves);
      *list++ = (to << 6) | (to - 7);
    }

  // White pawn captures

    bbMoves = ((PcBb(p, White, Pawn) & ~FILE_H_BB & ~RANK_7_BB) << 9) & p->cl_bb[Black];
    while (bbMoves) {
      to = PopFirstBit(&bbMoves);
      *list++ = (to << 6) | (to - 9);
    }

  // White en passant capture

    if ((to = p->ep_sq) != NO_SQ) {
      if (((PcBb(p, White, Pawn) & ~FILE_A_BB) << 7) & SqBb(to))
        *list++ = (EP_CAP << 12) | (to << 6) | (to - 7);
      if (((PcBb(p, White, Pawn) & ~FILE_H_BB) << 9) & SqBb(to))
        *list++ = (EP_CAP << 12) | (to << 6) | (to - 9);
    }
  } else {

    // Black pawn promotions with capture

    bbMoves = ((PcBb(p, Black, Pawn) & ~FILE_A_BB & RANK_2_BB) >> 9) & p->cl_bb[White];
    while (bbMoves) {
      to = PopFirstBit(&bbMoves);
      list = SerializePromotions(list, to + 9, to);
    }

  // Black pawn promotions with capture

    bbMoves = ((PcBb(p, Black, Pawn) & ~FILE_H_BB & RANK_2_BB) >> 7) & p->cl_bb[White];
    while (bbMoves) {
      to = PopFirstBit(&bbMoves);
      list = SerializePromotions(list, to + 7, to);
    }

  // Black pawn promotions

    bbMoves = ((PcBb(p, Black, Pawn) & RANK_2_BB) >> 8) & UnoccBb(p);
    while (bbMoves) {
      to = PopFirstBit(&bbMoves);
      list = SerializePromotions(list, to + 8, to);
    }

  // Black pawn captures, excluding promotions

    bbMoves = ((PcBb(p, Black, Pawn) & ~FILE_A_BB & ~RANK_2_BB) >> 9) & bbEnemy;
    while (bbMoves) { 
      to = PopFirstBit(&bbMoves);
      *list++ = (to << 6) | (to + 9);
    }

  // Black pawn captures, excluding promotions

    bbMoves = ((PcBb(p, Black, Pawn) & ~FILE_H_BB & ~RANK_2_BB) >> 7) & bbEnemy;
    while (bbMoves) {
      to = PopFirstBit(&bbMoves);
      *list++ = (to << 6) | (to + 7);
    }

  // Black en passant capture

    if ((to = p->ep_sq) != NO_SQ) {
      if (((p->Map(Black, Pawn) & ~FILE_A_BB) >> 9) & SqBb(to))
        *list++ = (EP_CAP << 12) | (to << 6) | (to + 9);
      if (((p->Map(Black, Pawn) & ~FILE_H_BB) >> 7) & SqBb(to))
        *list++ = (EP_CAP << 12) | (to << 6) | (to + 7);
    }
  }

  // Captures by knight

  bbPieces = p->Map(side, Knight);
  while (bbPieces) {
    from = PopFirstBit(&bbPieces);
    bbMoves = n_attacks[from] & bbEnemy;
    list = SerializeMoves(list, bbMoves, from);
  }

  // Captures by bishop

  bbPieces = p->Map(side, Bishop);
  while (bbPieces) {
    from = PopFirstBit(&bbPieces);
    bbMoves = BAttacks(OccBb(p), from) & bbEnemy;
    list = SerializeMoves(list, bbMoves, from);
  }

  // Captures by rook

  bbPieces = p->Map(side, Rook);
  while (bbPieces) {
    from = PopFirstBit(&bbPieces);
    bbMoves = RAttacks(OccBb(p), from) & bbEnemy;
    list = SerializeMoves(list, bbMoves, from);
  }

  // Captures by queen

  bbPieces = p->Map(side, Queen);
  while (bbPieces) {
    from = PopFirstBit(&bbPieces);
    bbMoves = QAttacks(OccBb(p), from) & bbEnemy;
    list = SerializeMoves(list, bbMoves, from);
  }

  // Captures by king

  bbMoves = k_attacks[KingSq(p, side)] & bbEnemy;
  list = SerializeMoves(list, bbMoves, KingSq(p, side));
  return list;
}

int *GenerateQuiet(Position *p, int *list) {

  U64 bbPieces, bbMoves;
  int from, to;
  int side = p->side;

  if (side == White) {

    // White short castle

    if ((p->castle_flags & 1) && !(OccBb(p) & (U64)0x0000000000000060))
      if (!Attacked(p, E1, Black) && !Attacked(p, F1, Black))
        *list++ = (CASTLE << 12) | (G1 << 6) | E1;

  // White long castle

    if ((p->castle_flags & 2) && !(OccBb(p) & (U64)0x000000000000000E))
      if (!Attacked(p, E1, Black) && !Attacked(p, D1, Black))
        *list++ = (CASTLE << 12) | (C1 << 6) | E1;

  // White double pawn moves

    bbMoves = ((((PcBb(p, White, Pawn) & RANK_2_BB) << 8) & UnoccBb(p)) << 8) & UnoccBb(p);
    while (bbMoves) {
      to = PopFirstBit(&bbMoves);
      *list++ = (EP_SET << 12) | (to << 6) | (to - 16);
    }

  // White normal pawn moves

    bbMoves = ((PcBb(p, White, Pawn) & ~RANK_7_BB) << 8) & UnoccBb(p);
    while (bbMoves) {
      to = PopFirstBit(&bbMoves);
      *list++ = (to << 6) | (to - 8);
    }
  } else {

    // Black short castle

    if ((p->castle_flags & 4) && !(OccBb(p) & (U64)0x6000000000000000))
      if (!Attacked(p, E8, White) && !Attacked(p, F8, White))
        *list++ = (CASTLE << 12) | (G8 << 6) | E8;

  // Black long castle

    if ((p->castle_flags & 8) && !(OccBb(p) & (U64)0x0E00000000000000))
      if (!Attacked(p, E8, White) && !Attacked(p, D8, White))
        *list++ = (CASTLE << 12) | (C8 << 6) | E8;

  // Black double pawn moves

    bbMoves = ((((PcBb(p, Black, Pawn) & RANK_7_BB) >> 8) & UnoccBb(p)) >> 8) & UnoccBb(p);
    while (bbMoves) {
      to = PopFirstBit(&bbMoves);
      *list++ = (EP_SET << 12) | (to << 6) | (to + 16);
    }

  // Black single pawn moves

    bbMoves = ((PcBb(p, Black, Pawn) & ~RANK_2_BB) >> 8) & UnoccBb(p);
    while (bbMoves) {
      to = PopFirstBit(&bbMoves);
      *list++ = (to << 6) | (to + 8);
    }
  }

  // Knight moves

  bbPieces = PcBb(p, side, Knight);
  while (bbPieces) {
    from = PopFirstBit(&bbPieces);
    bbMoves = n_attacks[from] & UnoccBb(p);
    list = SerializeMoves(list, bbMoves, from);
  }

  // Bishop moves

  bbPieces = PcBb(p, side, Bishop);
  while (bbPieces) {
    from = PopFirstBit(&bbPieces);
    bbMoves = BAttacks(OccBb(p), from) & UnoccBb(p);
    list = SerializeMoves(list, bbMoves, from);
  }

  // Rook moves

  bbPieces = PcBb(p, side, Rook);
  while (bbPieces) {
    from = PopFirstBit(&bbPieces);
    bbMoves = RAttacks(OccBb(p), from) & UnoccBb(p);
    list = SerializeMoves(list, bbMoves, from);
  }

  // Queen moves

  bbPieces = p->Map(side, Queen);
  while (bbPieces) {
    from = PopFirstBit(&bbPieces);
    bbMoves = QAttacks(OccBb(p), from) & UnoccBb(p);
    list = SerializeMoves(list, bbMoves, from);
  }

  // King moves

  bbMoves = k_attacks[KingSq(p, side)] & UnoccBb(p);
  list = SerializeMoves(list, bbMoves, KingSq(p, side));

  return list;
}

int* SerializeMoves(int* list, U64 moves, int from)
{
    while (moves) {
        int to = PopFirstBit(&moves);
        *list++ = CreateMove(from, to);
    }

    return list;
}

int* SerializePromotions(int* list, int from, int to)
{
    *list++ = CreateMove(from, to, Q_PROM);
    *list++ = CreateMove(from, to, R_PROM);
    *list++ = CreateMove(from, to, B_PROM);
    *list++ = CreateMove(from, to, N_PROM);

    return list;
}

int CreateMove(int from, int to, int flag)
{
    return (flag << 12) | (to << 6) | from;
}

int CreateMove(int from, int to)
{
    return (to << 6) | from;
}


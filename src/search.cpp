#include <stdio.h>
#include <string.h>
#include "lizard.h"
#include "timer.h"

int oldEval[MAX_PLY];

double lmrSize[2][MAX_PLY][MAX_MOVES];

const int seeDepth = 3;

int Search(Position *p, int ply, int alpha, int beta, int depth, int was_null, int *pv) {

  int best, score, move, newDepth, newPv[MAX_PLY];
  int isInCheck, isNodePrunable, isMovePrunable, moveType, reduction;
  int isPvNode = (beta > alpha + 1);
  int movesTried = 0, quietMovesTried = 0, canDoFutility = 0;
  int moveList[256];
  int histScore = 0;
  int seeScore = 0;

  MOVES m[1];
  UNDO undoData[1];

  // Quiescence search entry point

  if (depth <= 0) 
      return Quiesce(p, ply, alpha, beta, pv);

  // Periodically check for timeout, ponderhit or stop command

  nodes++;
  CheckForTimeout();
 //if (nodes % 100000 == 0)
 //     Accumulator.SetFromScratch(p);

  // Quick exit on a timeout or on a statically detected draw
  
  if (abort_search) return 0;
  if (ply) *pv = 0;
  if (IsDraw(p) && ply) return 0;

  // Retrieving data from transposition table. We hope for a cutoff
  // or at least for a move to improve move ordering.

  move = 0;
  if (TransRetrieve(p->hash_key, &move, &score, alpha, beta, depth, ply)) {
    
    // For move ordering purposes, a cutoff from hash is treated
    // exactly like a cutoff from search

      if (score >= beta) {
          UpdateHistory(p, move, depth, ply);
          UpdateTried(p, move, depth);
      }

    // In pv nodes only exact scores are returned. This is done because
    // there is much more pruning and reductions in zero-window nodes,
    // so retrieving such scores in pv nodes works like retrieving scores
    // from slightly lower depth.

    if (!isPvNode || (score > alpha && score < beta))
      return score;
  }
  
  // Safeguard against exceeding ply limit
  
  if (ply >= MAX_PLY - 1)
    return Evaluate(p);

  // Are we in check? Knowing that is useful when it comes 
  // to pruning/reduction decisions

  isInCheck = InCheck(p);
  isNodePrunable = !isInCheck & !isPvNode;

  // Set current node's eval
  // and remember this score for pruning/reduction  decisions

  int eval = -INF;
  if (!isInCheck)
      eval = Evaluate(p);

  oldEval[ply] = eval;

  // Has our score improved from two moves ago?
  // default is the state in which we reduce less

  bool improving = true;

  if (ply > 1 && !isInCheck) {
      if (eval <= oldEval[ply - 2])
          improving = false;
  }

  // Node level pruning

  if (isNodePrunable
  && !was_null
  && MayNull(p)) {

    // Static null move

    if (depth <= 3 && eval - 150 * depth > beta) {
        return eval - 150 * depth;
    }

    // Null move

    if (depth > 1 && eval > beta) {
      reduction = 3;
      if (depth > 8) reduction += depth / 4;

      p->DoNull(undoData);
      score = -Search(p, ply + 1, -beta, -beta + 1, depth - reduction, 1, newPv);
      p->UndoNull(undoData);

      if (abort_search ) return 0;
      if (score >= beta) return score;
    }
  } 
  
  // end of null move code

  // Razoring based on Toga II 3.0

  if (isNodePrunable
  && !move
  && !was_null
  && !(p->Map(p->side, Pawn) & bbRelRank[p->side][RANK_7]) // no pawns to promote in one move
  &&  depth <= 3) {
    int threshold = beta - 300 - (depth - 1) * 60;

    if (eval < threshold) {
      score = Quiesce(p, ply, alpha, beta, pv);
      if (score < threshold) return score;
    }
  } 
  
  // end of razoring code

  // Set futility pruning flag

  if (depth <= 6
  && isNodePrunable) {
    if (Evaluate(p) + 50 + 50 * depth < beta) 
        canDoFutility = 1;
  }

  // Init moves and variables before entering main loop
  
  best = -INF;
  InitMoves(p, m, move, ply);
  
  // Main loop
  
  while ((move = NextMove(m, &moveType))) {

      if (moveType == MV_BADCAPT && depth <= seeDepth)
      {
          seeScore = Swap(p, Fsq(move), Tsq(move));
      }
  
    p->DoMove(move, undoData);
    if (Illegal(p)) { p->UndoMove(move, undoData); continue; }

  // Update move statistics (needed for reduction/pruning decisions)

  moveList[movesTried] = move;
  movesTried++;
  if (moveType == MV_NORMAL)
      quietMovesTried++;
  histScore = GetHistory(p->pc[Fsq(move)], Tsq(move));
  isMovePrunable = !InCheck(p) && (moveType == MV_NORMAL) && histScore < 900;

  // Set new search depth

  newDepth = depth - 1 + InCheck(p);

  // Futility pruning

  if (canDoFutility
  &&  isMovePrunable
  &&  movesTried > 1) {
    p->UndoMove(move, undoData); continue;
  }

  // Late move pruning

  if (isNodePrunable
  && quietMovesTried > 4 * depth
  && isMovePrunable
  && depth <= 3
  && MoveType(move) != CASTLE ) {
    p->UndoMove(move, undoData); continue;
  }

  // SEE pruning of bad captures

  if (!isInCheck
      && !InCheck(p)
      && moveType == MV_BADCAPT
      && depth <= seeDepth
      && !isPvNode)
  {
      
      if (seeScore < -200 * depth) 
      {
          p->UndoMove(move, undoData);
          continue;
      }
  }

  // Late move reduction

  reduction = 0;

  if (depth >= 2 
  && movesTried > 3
  && !isInCheck 
  &&  isMovePrunable
  && lmrSize[isPvNode][depth][movesTried] > 0
  && MoveType(move) != CASTLE ) 
  {
    reduction = lmrSize[isPvNode][depth][movesTried];
    //if (histScore < 100)
    //    reduction++;
    if (!isPvNode && !improving) reduction++;
    newDepth -= reduction;
  }

  re_search:
   
  // PVS

    if (best == -INF)
      score = -Search(p, ply + 1, -beta, -alpha, newDepth, 0, newPv);
    else {
      score = -Search(p, ply + 1, -alpha - 1, -alpha, newDepth, 0, newPv);
      if (!abort_search && score > alpha && score < beta)
        score = -Search(p, ply + 1, -beta, -alpha, newDepth, 0, newPv);
    }

  // Reduced move scored above alpha - we need to re-search it

  if (reduction && score > alpha) {
    newDepth += reduction;
    reduction = 0;
    goto re_search;
  }

    p->UndoMove(move, undoData);
    if (abort_search) return 0;

  // Beta cutoff

    if (score >= beta) {
      UpdateHistory(p, move, depth, ply);
      for (int x = 0; x < movesTried; x++)
          UpdateTried(p, moveList[x], depth);
      TransStore(p->hash_key, move, score, LOWER, depth, ply);

	  // If beta cutoff occurs at the root, change the best move

	  if (!ply) {
		  BuildPv(pv, newPv, move);
		  DisplayPv(score, pv);
	  }

      return score;
    }

  // Updating score and alpha

    if (score > best) {
      best = score;
      if (score > alpha) {
        alpha = score;
        BuildPv(pv, newPv, move);
        if (!ply) DisplayPv(score, pv);
      }
    }

  } // end of the main loop

  // Return correct checkmate/stalemate score

  if (best == -INF)
    return InCheck(p) ? -MATE + ply : 0;

  // Save score in the transposition table

  if (*pv) {
    TransStore(p->hash_key, *pv, best, EXACT, depth, ply);
  } else
    TransStore(p->hash_key, 0, best, UPPER, depth, ply);

  return best;
}

int IsDraw(Position *p) {

  // Draw by 50 move rule

  if (p->rev_moves > 100) return 1;

  // Draw by repetition

  for (int i = 4; i <= p->rev_moves; i += 2)
    if (p->hash_key == p->rep_list[p->head - i])
      return 1;

  // Draw by insufficient material (bare kings or Km vs K)

  if (!Illegal(p)) {
    if (p->cnt[White][Pawn] + p->cnt[Black][Pawn] + p->cnt[White][Queen] + p->cnt[Black][Queen] + p->cnt[White][Rook] + p->cnt[Black][Rook] == 0
    &&  p->cnt[White][Knight] + p->cnt[Black][Knight] + p->cnt[White][Bishop] + p->cnt[Black][Bishop] <= 1) return 0;
  }

  return 0; // default: no draw
}

void DisplayPv(int score, int *pv) {

  char *type, pv_str[512];
  U64 nps = 0;
  int elapsed = Timer.GetElapsedTime();
  if (elapsed) nps = nodes * 1000 / elapsed;

  type = "mate";
  if (score < -MAX_EVAL)
    score = (-MATE - score) / 2;
  else if (score > MAX_EVAL)
    score = (MATE - score + 1) / 2;
  else
    type = "cp";

  PvToStr(pv, pv_str);
  printf("info depth %d time %d nodes %I64d nps %I64d score %s %d pv %s\n",
      root_depth, elapsed, nodes, nps, type, score, pv_str);
}

void CheckForTimeout(void) {

  char command[80];

  if (nodes & 4095 || root_depth == 1)
    return;

  if (InputAvailable()) {
    ReadLine(command, sizeof(command));

    if (strcmp(command, "stop") == 0)
      abort_search = 1;
    else if (strcmp(command, "ponderhit") == 0)
      isPondering = 0;
  }

  if (Timeout()) abort_search = 1;
}

int Timeout() {

  return (!isPondering && !Timer.IsInfiniteMode() && Timer.TimeHasElapsed());
}

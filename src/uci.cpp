#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lizard.h"
#include "timer.h"
#include <ctime>

#ifdef USE_TUNING
cTuner Tuner;
#endif

void ReadLine(char *str, int n) {
  char *ptr;

  if (fgets(str, n, stdin) == NULL)
    exit(0);
  if ((ptr = strchr(str, '\n')) != NULL)
    *ptr = '\0';
}

char *ParseToken(char *string, char *token) {

  while (*string == ' ')
    string++;
  while (*string != ' ' && *string != '\0')
    *token++ = *string++;
  *token = '\0';
  return string;
}

void UciLoop(void) {

  char command[4096], token[80], *ptr;
  Position p[1];

  setbuf(stdin, NULL);
  setbuf(stdout, NULL);
  SetPosition(p, START_POS);
  AllocTrans(16);
  for (;;) {
    ReadLine(command, sizeof(command));
    ptr = ParseToken(command, token);
    if (strcmp(token, "uci") == 0) {
      printf("id name LizardBrain\n");
      printf("id author Pawel Koziol (based on Sungorus 1.4 by Pablo Vazquez)\n");
      printf("option name Hash type spin default 16 min 1 max 4096\n");
     printf("uciok\n");
    } else if (strcmp(token, "isready") == 0) {
      printf("readyok\n");
    } else if (strcmp(token, "setoption") == 0) {
      ParseSetoption(ptr);
    } else if (strcmp(token, "position") == 0) {
      ParsePosition(p, ptr);
    } else if (strcmp(token, "perft") == 0) {
      ptr = ParseToken(ptr, token);
	  int depth = atoi(token);
	  if (depth == 0) depth = 5;
	  Timer.SetStartTime();
	  nodes = Perft(p, 0, depth);
	  printf (" perft %d : %d nodes in %d miliseconds\n", depth, nodes, Timer.GetElapsedTime() );
    } else if (strcmp(token, "print") == 0) {
      PrintBoard(p);
    } else if (strcmp(token, "step") == 0) {
      ParseMoves(p, ptr);
    } else if (strcmp(token, "go") == 0) {
      ParseGo(p, ptr);
    }
    else if (strcmp(token, "bench") == 0) {
        ptr = ParseToken(ptr, token);
        Bench(atoi(token));
    }
    else if (strcmp(token, "train") == 0) {
        for (int i = 0; i < numberOfBatches; i++) {
            printf("BATCH %d\n", i+1);
            OnTrainComand(p);
        }
    } else if (strcmp(token, "fit") == 0) {
#ifdef USE_TUNING
        int pv[MAX_PLY];
        Tuner.Init(1000);
        printf("info string current fit: %lf\n", Tuner.TexelFit(p, pv));
#endif
    } else if (strcmp(token, "quit") == 0) {
      exit(0);
    }
  }
}

void ParseSetoption(char *ptr) {

  char token[80], name[80], value[80] = "";

  ptr = ParseToken(ptr, token);
  name[0] = '\0';
  for (;;) {
    ptr = ParseToken(ptr, token);
    if (*token == '\0' || strcmp(token, "value") == 0)
      break;
    strcat(name, token);
    strcat(name, " ");
  }
  name[strlen(name) - 1] = '\0';
  if (strcmp(token, "value") == 0) {
    value[0] = '\0';

    for (;;) {
      ptr = ParseToken(ptr, token);
      if (*token == '\0')
        break;
      strcat(value, token);
      strcat(value, " ");
    }
    value[strlen(value) - 1] = '\0';
  }

  if (strcmp(name, "Hash") == 0) {
    AllocTrans(atoi(value));
  }
  else if (strcmp(name, "Clear Hash") == 0) {
      ResetEngine();
  }
}

void ParseMoves(Position *p, char *ptr) {
	
  char token[80];
  UNDO u[1];

  for (;;) {

    // Get next move to parse

    ptr = ParseToken(ptr, token);

	// No more moves!

    if (*token == '\0') break;

    p->DoMove(StrToMove(p, token), u);

	// We won't be taking back moves beyond this point:

    if (p->rev_moves == 0) p->head = 0;
  }
}

void ParsePosition(Position *p, char *ptr) {

  char token[80], fen[80];

  ptr = ParseToken(ptr, token);
  if (strcmp(token, "fen") == 0) {
    fen[0] = '\0';
    for (;;) {
      ptr = ParseToken(ptr, token);

      if (*token == '\0' || strcmp(token, "moves") == 0)
        break;

      strcat(fen, token);
      strcat(fen, " ");
    }
    SetPosition(p, fen);
  } else {
    ptr = ParseToken(ptr, token);
    SetPosition(p, START_POS);
  }

  if (strcmp(token, "moves") == 0)
    ParseMoves(p, ptr);
}

void ParseGo(Position *p, char *ptr) {

  char token[80], bestmove_str[6], ponder_str[6];
  int pv[MAX_PLY];

  Timer.Clear();
  isPondering = 0;

  for (;;) {
    ptr = ParseToken(ptr, token);
    if (*token == '\0')
      break;
    if (strcmp(token, "ponder") == 0) {
      isPondering = 1;
    } else if (strcmp(token, "wtime") == 0) {
      ptr = ParseToken(ptr, token);
      Timer.SetData(W_TIME, atoi(token));
    } else if (strcmp(token, "btime") == 0) {
      ptr = ParseToken(ptr, token);
      Timer.SetData(B_TIME, atoi(token));
    } else if (strcmp(token, "winc") == 0) {
      ptr = ParseToken(ptr, token);
      Timer.SetData(W_INC, atoi(token));
    } else if (strcmp(token, "binc") == 0) {
      ptr = ParseToken(ptr, token);
      Timer.SetData(B_INC, atoi(token));
    } else if (strcmp(token, "movestogo") == 0) {
      ptr = ParseToken(ptr, token);
      Timer.SetData(MOVES_TO_GO, atoi(token));
    } else if (strcmp(token, "depth") == 0) {
      ptr = ParseToken(ptr, token);
      Timer.SetData(FLAG_INFINITE, 1);
      Timer.SetData(MAX_DEPTH, atoi(token));
    } else if (strcmp(token, "infinite") == 0) {
      Timer.SetData(FLAG_INFINITE, 1);
    }
  }

  Timer.SetSideData(p->side);
  Timer.SetMoveTiming();
  Think(p, pv);
  MoveToStr(pv[0], bestmove_str);
  if (pv[1]) {
    MoveToStr(pv[1], ponder_str);
    printf("bestmove %s ponder %s\n", bestmove_str, ponder_str);
  } else
    printf("bestmove %s\n", bestmove_str);
}

void ResetEngine(void) {

  ClearHist();
  ClearTrans();
}

void OnTrainComand(Position* p) {

#ifdef USE_TUNING
    int pv[MAX_PLY];
    Tuner.Init(5);
    double best = Tuner.TexelFit(p, pv);
    Network.Init(0);
    srand(time(0));

    for (int i = 0; i < changesPerBatch; i++) {
        Network.PerturbWeight(weightChange);
        double fit = Tuner.TexelFit(p, pv);
        if (fit < best) {
            best = fit;
            Network.SaveWeights("lizard_weights.bin");
            printf("step %d fit %lf\n", i, fit);
        } else {
            Network.LoadWeights("lizard_weights.bin");
        }
    }
#endif
}
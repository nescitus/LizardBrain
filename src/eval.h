
struct sEvalHashEntry {
  U64 key;
  int score;
};

#define EVAL_HASH_SIZE 512*512
extern sEvalHashEntry EvalTT[EVAL_HASH_SIZE];
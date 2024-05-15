#include "lizard.h"
#include "timer.h"

sTimer Timer; // class for setting and observing time limits
cNetwork Network;
cAccumulator Accumulator;

int main() {
#ifdef USE_TUNING
	Tuner.LoadAll();
#endif
  Init();
  Network.Init(0);
  InitLmr();
  UciLoop();
  return 0;
}

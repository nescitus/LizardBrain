#include "lizard.h"
#include "timer.h"

sTimer Timer; // class for setting and observing time limits
cNetwork Network;

int main() {
  Init();
  Network.Init(0);
  InitLmr();
  UciLoop();
  return 0;
}

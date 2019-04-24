#include <Arduino.h>
#include "main.h"

void setup() {

  char buf[256];
  sprintf(buf, "FPGAArcade Replay VIDOR %s %s", __DATE__, __TIME__);

  Serial1.begin(115200);
  Serial1.println("\033[2J");
  Serial1.println("=========================================================");
  Serial1.println(buf);
  Serial1.println("=========================================================");
  Serial1.flush();
}

void loop() {
  replay_main();
}

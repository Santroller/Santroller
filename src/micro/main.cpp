#include "../shared/controller/input/InputHandler.h"
#include "../shared/controller/output/OutputHandler.h"
#include "../shared/sharedmain.h"
#include "pins.h"
InputHandler controller;
OutputHandler out;
Main main2;

int main() {
  check_freq();
  main2.main();
  out.init();
  controller.init();
  while (true) {
    controller.process();
    out.process(&controller.controller);
  }
}
extern "C" void before_reboot() {}
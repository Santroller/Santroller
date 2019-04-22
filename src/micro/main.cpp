#include "../shared/controller/input/InputHandler.h"
#include "../shared/controller/output/OutputHandler.h"
#include "pins.h"

InputHandler controller;
OutputHandler out;

int main() {
  out.init();
  controller.init();
  while (true) {
    if (out.ready()) {
      controller.process();
    }
    out.process(&controller.controller);
  }
}
#include "../shared/controller/InputHandler.h"
#include "../shared/controller/output/OutputHandler.h"
#include "../shared/wii/WiiExtension.h"
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
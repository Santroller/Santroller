#include "../shared/controller/input/InputHandler.h"
#include "../shared/controller/output/OutputHandler.h"
InputHandler controller;
OutputHandler out;

int main() {
  load_config();
  out.init();
  controller.init();
  while (true) {
    controller.process();
    out.process(&controller.controller);
  }
}
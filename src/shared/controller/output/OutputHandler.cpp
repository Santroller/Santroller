#include "OutputHandler.h"
void OutputHandler::process(Controller *controller)
{
  if (bit_check(controller->buttons, START) &&
      bit_check(controller->buttons, SELECT))
  {
    bootloader();
  }
  output.update(*controller);
}

void OutputHandler::init()
{
  output.init();
}
extern "C" {
  #include "XInputPad.h"
  #include "util.h"
}
#include <NintendoExtensionCtrl.h>
#include "WiiExtension.h"
WiiExtension controller;
void setup()
{
  xbox_init(true);
  // Disable JTAG
  bit_set(MCUCR, 1 << JTD);
  bit_set(MCUCR, 1 << JTD);
  pinMode(4, INPUT_PULLUP); 
  controller.setup();
  pinMode(21, OUTPUT);
  pinMode(15, OUTPUT);
  digitalWrite(21, HIGH);
  digitalWrite(15, LOW);
}
void loop()
{
  xbox_reset_watchdog();
  controller.read_controller((WiiController*)&gamepad_state);
  xbox_send_pad_state();
}

#include "DualVirtualSerial.h"
#include "ArduinoSerial.h"

LUFACDCSerial SerialU1(&VirtualSerial1_CDC_Interface);
LUFACDCSerial SerialU2(&VirtualSerial2_CDC_Interface);

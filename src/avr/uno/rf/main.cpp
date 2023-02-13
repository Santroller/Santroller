#include <Arduino.h>
#include <SPI.h>
#include <avr/io.h>
#include <util/delay.h>

#include "Usb.h"
#include "config.h"
#include "controller_reports.h"
#include "defines.h"
#include "descriptors.h"
#include "hid.h"
#include "packets.h"
#include "shared_main.h"


void setup() {
    init_main();
}
USB_Report_Data_t report;
void loop() {
    
}


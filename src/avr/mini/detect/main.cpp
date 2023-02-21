#include <Arduino.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/power.h>
#include <avr/wdt.h>
#include <string.h>
uint8_t realFreq = 16;
char freqString[10];
volatile bool waiting = true;
void setup() {
    sei();  // enable global interrupts
    wdt_reset();
    // Enable the watchdog timer, as it runs from an internal clock so it will not be affected by the crystal
    MCUSR &= ~_BV(WDRF);
    /* Start the WDT Config change sequence. */
    WDTCSR |= _BV(WDCE) | _BV(WDE);
    /* Configure the prescaler and the WDT for interrupt mode only*/
    WDTCSR = _BV(WDIE) | WDTO_15MS;
    sei();
    long timeSinceWDT = millis();
    while (waiting) {
    }
    timeSinceWDT = millis() - timeSinceWDT;
    // And now compare to what we expect
    if (timeSinceWDT < 10) {
        // if the user is running at 8mhz, then it will run at around half speed, thus it will take less than 15ms
        realFreq = 8;
        // Since we are running at the wrong speed, we need to double the baud rate to get the correct characters out
        Serial.begin(115200 * 2);
    } else {
        Serial.begin(115200);
    }
    itoa(realFreq, freqString, 10);
    freqString[strlen(freqString)] = '\n';
}

void loop() {
    Serial.write(freqString);
}

ISR(WDT_vect) {
    wdt_disable();
    waiting = false;
}
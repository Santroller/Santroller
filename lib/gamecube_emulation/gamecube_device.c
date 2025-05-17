#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"

#include "joybus.pio.h"
#include "gamecube_device.h"

#define ORG 127

void __time_critical_func(convertToPio)(const uint8_t* command, const int len, uint32_t* result, int *resultLen) {
    // PIO Shifts to the right by default
    // In: pushes batches of 8 shifted left, i.e we get [0x40, 0x03, rumble (the end bit is never pushed)]
    // Out: We push commands for a right shift with an enable pin, ie 5 (101) would be 0b11'10'11
    // So in doesn't need post processing but out does
    if (len == 0) {
        *resultLen = 0;
        return;
    }
    *resultLen = len/2 + 1;
    int i;
    for (i = 0; i < *resultLen; i++) {
        result[i] = 0;
    }
    for (i = 0; i < len; i++) {
        for (int j = 0; j < 8; j++) {
            result[i / 2] += 1 << (2 * (8 * (i % 2) + j) + 1);
            result[i / 2] += (!!(command[i] & (0x80u >> j))) << (2 * (8 * (i % 2) + j));
        }
    }
    // End bit
    result[len / 2] += 3 << (2 * (8 * (len % 2)));
}


void gc_device_main(uint pio, GCReport *data, int data_pin){
    
    volatile GCReport *gcReport = data;

    /*Which pio use*/
    PIO gamecube_pio = pio ? pio1 : pio0;

    gpio_init(data_pin);
    gpio_set_dir(data_pin, GPIO_IN);
    gpio_pull_up(data_pin);

    sleep_us(100); // Stabilize voltages

    /*CONFIG PIO*/
    uint offset = pio_add_program(gamecube_pio, &joybus_program);
    uint sm = pio_claim_unused_sm(gamecube_pio, true);
    pio_sm_config config = joybus_program_get_default_config(offset);
    /* Mapping */
    pio_gpio_init(gamecube_pio, data_pin);
    sm_config_set_in_pins(&config, data_pin);
    sm_config_set_out_pins(&config, data_pin, 1);
    sm_config_set_set_pins(&config, data_pin, 1);
    // Set clock to 25MHz
    float div = (float)clock_get_hz(clk_sys) / (25e6);
    sm_config_set_clkdiv(&config, div);
    sm_config_set_out_shift(&config, true, false, 32);
    sm_config_set_in_shift(&config, false, true, 8);
    
    pio_sm_init(gamecube_pio, sm, offset, &config);
    pio_sm_set_enabled(gamecube_pio, sm, true);
    
    while (true) {
        uint8_t buffer[3];
        buffer[0] = pio_sm_get_blocking(gamecube_pio, sm);
        if (buffer[0] == 0) { // Probe
            uint8_t probeResponse[3] = { 0x09, 0x00, 0x03 };
            uint32_t result[2];
            int resultLen;
            convertToPio(probeResponse, 3, result, &resultLen);
            sleep_us(6); // 3.75us into the bit before end bit => 6.25 to wait if the end-bit is 5us long

            pio_sm_set_enabled(gamecube_pio, sm, false);
            pio_sm_init(gamecube_pio, sm, offset+joybus_offset_outmode, &config);
            pio_sm_set_enabled(gamecube_pio, sm, true);

            for (int i = 0; i<resultLen; i++) pio_sm_put_blocking(gamecube_pio, sm, result[i]);
        }
        else if (buffer[0] == 0x41) { // Origin (NOT 0x81)
            gpio_put(25, 1);
            uint8_t originResponse[10] = { 0x00, 0x80, ORG, ORG, ORG, ORG, 0, 0, 0, 0 };
            // TODO The origin response sends centered values in this code excerpt. Consider whether that makes sense for your project (digital controllers -> yes)
            uint32_t result[6];
            int resultLen;
            convertToPio(originResponse, 10, result, &resultLen);
            // Here we don't wait because convertToPio takes time

            pio_sm_set_enabled(gamecube_pio, sm, false);
            pio_sm_init(gamecube_pio, sm, offset+joybus_offset_outmode, &config);
            pio_sm_set_enabled(gamecube_pio, sm, true);

            for (int i = 0; i<resultLen; i++) pio_sm_put_blocking(gamecube_pio, sm, result[i]);
        }
        else if (buffer[0] == 0x40) { // Could check values past the first byte for reliability
            //The call to the state building function happens here, because on digital controllers, it's near instant, so it can be done between the poll and the response
            // It must be very fast (few us max) to be done between poll and response and still be compatible with adapters
            // Consider whether that makes sense for your project. If your state building is long, use a different control flow i.e precompute somehow and have func read it
            //GCReport gcReport = func();

            //get the second byte; we do this interleaved with work that must be done
            buffer[0] = pio_sm_get_blocking(gamecube_pio, sm);

            uint32_t result[5];
            int resultLen;
            convertToPio((uint8_t*)(gcReport), 8, result, &resultLen);

            //get the third byte; we do this interleaved with work that must be done
            buffer[0] = pio_sm_get_blocking(gamecube_pio, sm);

            //sleep_us(4);//add delay so we don't overwrite the stop bit
            sleep_us(7);//add delay so we don't overwrite the stop bit

            pio_sm_set_enabled(gamecube_pio, sm, false);
            pio_sm_init(gamecube_pio, sm, offset+joybus_offset_outmode, &config);
            pio_sm_set_enabled(gamecube_pio, sm, true);

            for (int i = 0; i<resultLen; i++) pio_sm_put_blocking(gamecube_pio, sm, result[i]);
        }
        else {
            pio_sm_set_enabled(gamecube_pio, sm, false);
            sleep_us(400);
            //If an unmatched communication happens, we wait for 400us for it to finish for sure before starting to listen again
            pio_sm_init(gamecube_pio, sm, offset+joybus_offset_inmode, &config);
            pio_sm_set_enabled(gamecube_pio, sm, true);
        }
    }
}

/*
RED -> DATA
BLACK -> GND
BLUE -> GND
WHITE -> 5
GREEN -> 3.3
*/

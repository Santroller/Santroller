#include "pico/stdlib.h"


typedef struct 
{
    uint8_t buttons1;
    uint8_t buttons2;
    uint8_t lx;
    uint8_t ly;
    uint8_t rx;
    uint8_t ry;
    uint8_t l;
    uint8_t r;
    
}GCReport;


// First byte
#define GC_GAMEPAD_A 0x01
#define GC_GAMEPAD_B 0x02
#define GC_GAMEPAD_X 0x04
#define GC_GAMEPAD_Y 0x08
#define GC_GAMEPAD_START 0x10
// Second byte
#define GC_GAMEPAD_DPAD_LEFT 0x01
#define GC_GAMEPAD_DPAD_RIGHT 0x02
#define GC_GAMEPAD_DPAD_DOWN 0x04
#define GC_GAMEPAD_DPAD_UP 0x08
#define GC_GAMEPAD_Z 0x10
#define GC_GAMEPAD_R 0x20
#define GC_GAMEPAD_L 0x40
#define GC_BYTE_2_1 0x80

// https://www.int03.co.uk/crema/hardware/gamecube/gc-control.html

void gc_device_main(uint pio, GCReport *data, int data_pin);

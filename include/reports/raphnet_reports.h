#include <stdint.h>
#define RAPHNET_MAX 32000
#define RAPHNET_CENTER 16000
typedef struct {
    uint8_t reportId;
    uint16_t joyX;
    uint16_t joyY;
    uint16_t unused;
    uint16_t slider;
    uint16_t unused2;
    uint16_t whammy;
    uint8_t green : 1;
    uint8_t red : 1;
    uint8_t yellow : 1;
    uint8_t blue : 1;
    uint8_t orange : 1;
    uint8_t up : 1;
    uint8_t plus : 1;
    uint8_t minus : 1;

    uint8_t down : 1;
    uint8_t : 7;

} __attribute__((packed)) RaphnetGuitar_Data_t;

typedef struct {
    uint8_t reportId;
    uint16_t joyX;
    uint16_t joyY;
    uint16_t unused[4];
    uint16_t green : 1;
    uint16_t red : 1;
    uint16_t yellow : 1;
    uint16_t blue : 1;
    uint16_t orange : 1;
    uint16_t plus : 1;
    uint16_t minus : 1;
    uint16_t : 9;
} __attribute__((packed)) RaphnetDrum_Data_t;

typedef struct {
    uint8_t reportId;
    uint16_t leftJoyX;
    uint16_t leftJoyY;
    uint16_t rightJoyX;
    uint16_t rightJoyY;
    uint16_t leftTrigger;
    uint16_t rightTrigger;
    uint8_t y : 1;
    uint8_t b : 1;
    uint8_t select : 1;
    uint8_t start : 1;
    uint8_t a : 1;
    uint8_t x : 1;
    uint8_t lTriggerDigital : 1;
    uint8_t rTriggerDigital : 1;

    uint8_t leftShoulder : 1;
    uint8_t rightShoulder : 1;
    uint8_t home : 1;
    uint8_t unused : 1;
    uint8_t up : 1;
    uint8_t down : 1;
    uint8_t left : 1;
    uint8_t right : 1;
} __attribute__((packed)) RaphnetGamepad_Data_t;
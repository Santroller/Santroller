#include <stdint.h>
typedef struct {
    uint8_t channel;
    uint8_t velocities[127];
} san_midi_t;
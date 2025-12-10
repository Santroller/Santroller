#include <stdint.h>

#include "Arduino.h"
#include "io.h"
#include "shared_main.h"
#include "commands.h"
#define GH5NECK_ADDR 0x0D
#define GH5NECK_BUTTONS_PTR 0x10
#ifdef GH5_TWI_PORT
bool lastGH5WasSuccessful = false;
void tickGh5Neck()
{
    uint8_t header[2];
    uint8_t data[2];
    lastGH5WasSuccessful = twi_readFromPointerRepeatedStart(GH5_TWI_PORT, GH5NECK_ADDR, GH5NECK_BUTTONS_PTR, sizeof(header), header);
    if (lastGH5WasSuccessful && header[1])
    {
        lastGH5WasSuccessful = twi_readFrom(GH5_TWI_PORT, GH5NECK_ADDR, data, sizeof(data), true);
        memcpy(lastSuccessfulGH5Packet, data, sizeof(data));
    }
}
#endif
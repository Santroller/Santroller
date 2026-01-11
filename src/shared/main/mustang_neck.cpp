#include <stdint.h>

#include "Arduino.h"
#include "io.h"
#include "shared_main.h"
#include "state_translation/pro_guitar.h"
#ifdef MUSTANG_NECK_SPI_PORT
static long lastTick = 0;
static int missing = 0;
bool mustang_neck_found = false;
static protarneck_t neck;
protarneck_t tickMustangNeck()
{
    if (micros() - lastTick > 500)
    {
        lastTick = micros();
        MUSTANG_NECK_CS_CLEAR();
        uint8_t resp = spi_transfer(MUSTANG_NECK_SPI_PORT, 0x80);
        if (resp == 0x00)
        {
            mustang_neck_found = true;
            memset(&neck, 0, sizeof(neck));
            MUSTANG_NECK_CS_SET();
            return neck;
        }
        if (resp != 0x80)
        {
            mustang_neck_found = false;
            MUSTANG_NECK_CS_SET();
            return neck;
        }
        mustang_neck_found = true;
        uint8_t *buf = (uint8_t *)&neck;
        for (int i = 0; i < sizeof(neck); i++)
        {
            buf[i] = spi_transfer(MUSTANG_NECK_SPI_PORT, 0x12);
        }
        MUSTANG_NECK_CS_SET();
    }
    return neck;
}
#endif
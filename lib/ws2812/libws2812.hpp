#include <pico/time.h>
#include <hardware/dma.h>
#include "ws2812.pio.h"
#include "enums.pb.h"
class WS2812
{
public:
    WS2812(uint8_t pin, uint8_t count, WS2812Type type);
    ~WS2812();
    void begin();
    void end();
    void putLeds(uint32_t *leds, uint8_t count);

private:
    uint8_t m_pin;
    bool hasW;
    WS2812Type m_type;
    PIO ws2812Pio;
    uint ws2812Sm;
    uint ws2812Offset;
    uint32_t* led_state;
    uint8_t m_count;
    uint m_dma_channel = UINT_MAX;
    alarm_id_t m_reset_delay_alarm_id = 0;

    static WS2812* s_dma_owners[NUM_DMA_CHANNELS];
    static int64_t reset_delay_complete(alarm_id_t id, void* user_data);
    static void dma_complete_handler();
};
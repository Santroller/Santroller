#include "pin_funcs.h"
#include "reports/controller_reports.h"
#include "config.h"

void init_main(void);

void device_reset(void);
void tick(void);
void tick_wiioutput();
uint8_t tick_inputs(USB_Host_Data_t *buf, USB_LastReport_Data_t *last_report, uint8_t output_console_type);
uint8_t tick_controllers(void *buf, USB_LastReport_Data_t *last_report, uint8_t output_console_type, USB_Host_Data_t *universal_report);
void reset_usb(void);
uint8_t transfer_with_usb_controller(const uint8_t dev_addr, const uint8_t requestType, const uint8_t request, const uint16_t wValue, const uint16_t wIndex, const uint16_t wLength, uint8_t *buffer);
void send_report_to_controller(uint8_t dev_addr, uint8_t instance, const uint8_t *report, uint8_t len);
void send_report_to_pc(const void *report, uint8_t len);
bool ready_for_next_packet(void);
bool usb_configured(void);
void receive_report_from_controller(uint8_t const *report, uint16_t len);
void xinput_controller_connected(uint16_t vid, uint16_t pid);
void xinput_w_controller_connected();
void xone_controller_connected(uint8_t dev_addr, uint8_t instance);
bool xone_controller_send_init_packet(uint8_t dev_addr, uint8_t instance, uint8_t id);
void ps4_controller_connected(uint8_t dev_addr, uint16_t vid, uint16_t pid);
void ps3_controller_connected(uint8_t dev_addr, uint16_t vid, uint16_t pid);
void ps4_controller_disconnected(void);
void host_controller_connected(void);
void on_connect(void);
void set_console_type(uint8_t new_console_type);
void authentication_successful(void);
void onNote(uint8_t channel, uint8_t note, uint8_t velocity);

void offNote(uint8_t channel, uint8_t note, uint8_t velocity);

void onControlChange(uint8_t channel, uint8_t b1, uint8_t b2);
void onPitchBend(uint8_t channel, int pitch);

typedef struct {
    uint8_t console_type;
    uint8_t sub_type;
    uint8_t dev_addr;
    uint8_t instance;
} USB_Device_Type_t;
void get_usb_device_type_for(uint16_t vid, uint16_t pid, uint16_t version, USB_Device_Type_t *type);
typedef struct {
    // If this bit is set, then an led effect (like star power) has overridden the leds
    uint8_t select;
    uint8_t brightness;
    uint8_t r;
    uint8_t g;
    uint8_t b;
} Led_t;

typedef struct {
    // If this bit is set, then an led effect (like star power) has overridden the leds
    uint8_t select;
    uint8_t r[4];
    uint8_t g[4];
    uint8_t b[4];
} __attribute__((packed)) Led_WS2812_t;

typedef struct {
    // If this bit is set, then an led effect (like star power) has overridden the leds
    uint8_t select;
    uint8_t r[4];
    uint8_t g[4];
    uint8_t b[4];
    uint8_t w[4];
} __attribute__((packed)) Led_WS2812W_t;
extern uint8_t ws2812_bits[4];
extern uint8_t led_tmp;
#define ROUND_UP(N, S) ((((N) + (S)-1) / (S)) * (S))
// for STP, two bits per led, select and on
#if LED_COUNT_WS2812W
extern Led_WS2812W_t ledState[LED_COUNT_WS2812W];
extern Led_WS2812W_t lastLedState[LED_COUNT_WS2812W];
#elif LED_COUNT_WS2812
extern Led_WS2812_t ledState[LED_COUNT_WS2812];
extern Led_WS2812_t lastLedState[LED_COUNT_WS2812];
#elif LED_COUNT_STP
extern uint8_t ledState[ROUND_UP(LED_COUNT_STP, 8) / 8];
extern uint8_t ledStateSelect[ROUND_UP(LED_COUNT_STP, 8) / 8];
extern uint8_t lastLedState[ROUND_UP(LED_COUNT_STP, 8) / 8];
#else
extern Led_t ledState[LED_COUNT];
extern Led_t lastLedState[LED_COUNT];
#endif

#if LED_COUNT_MPR121
extern uint8_t ledStateMpr121;
extern uint8_t ledStateMpr121Select;
extern uint8_t lastLedStateMpr121;
#endif
#if LED_COUNT_PERIPHERAL_WS2812W
extern Led_WS2812W_t ledStatePeripheral[LED_COUNT_PERIPHERAL_WS2812W];
extern Led_WS2812W_t lastLedStatePeripheral[LED_COUNT_PERIPHERAL_WS2812W];
#elif LED_COUNT_PERIPHERAL_WS2812
extern Led_WS2812_t ledStatePeripheral[LED_COUNT_PERIPHERAL_WS2812];
extern Led_WS2812_t lastLedStatePeripheral[LED_COUNT_PERIPHERAL_WS2812];
#elif LED_COUNT_PERIPHERAL_STP
extern uint8_t ledStatePeripheral[ROUND_UP(LED_COUNT_PERIPHERAL_STP, 8) / 8];
extern uint8_t ledStatePeripheralSelect[ROUND_UP(LED_COUNT_PERIPHERAL_STP, 8) / 8];
extern uint8_t lastLedStatePeripheral[ROUND_UP(LED_COUNT_PERIPHERAL_STP, 8) / 8];
#else
extern Led_t ledStatePeripheral[LED_COUNT_PERIPHERAL];
extern Led_t lastLedStatePeripheral[LED_COUNT_PERIPHERAL];
#endif
#if USB_HOST_STACK
USB_Device_Type_t get_device_address_for(uint8_t deviceType);
#endif
#ifdef INPUT_USB_HOST
void xone_disconnect(void);
uint8_t read_usb_host_devices(uint8_t *buf);
uint8_t get_usb_host_device_count();
USB_Device_Type_t get_usb_host_device_type(uint8_t id);
uint8_t get_usb_host_device_data(uint8_t id, uint8_t *buf);
extern USB_Host_Data_t usb_host_data;
extern USB_Host_Data_t last_usb_host_data;
extern USB_Host_Data_t lastSuccessfulPS2Packet;
extern USB_Host_Data_t lastSuccessfulWiiPacket;
#endif
extern USB_Host_Data_t bt_data;
extern USB_Host_Data_t last_report;
extern USB_LastReport_Data_t last_report_bt;


#if BLUETOOTH_RX
void tick_bluetooth(const void *buf, uint8_t len, USB_Device_Type_t type);
int tick_bluetooth_inputs(const void *buf, uint8_t len, USB_Device_Type_t type);
void bluetooth_connected(void);
#endif
extern uint8_t brightness;
#ifdef BLUETOOTH_TX
void set_battery_state(uint8_t state);
#endif
extern uint8_t wii_data[8];
uint8_t wii_data_format(void);
void handle_rumble(uint8_t rumble_left, uint8_t rumble_right);
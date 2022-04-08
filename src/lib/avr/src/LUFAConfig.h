#pragma once
#define ORDERED_EP_CONFIG
// On the 32u4, we set the PLL speed manually so that we can work out the correct speed at runtime, so that we don't have to hardcode a freq into the firmware
#ifdef __AVR_ATmega32U4__
#define USE_STATIC_OPTIONS \
    (USB_DEVICE_OPT_FULLSPEED | USB_OPT_REG_ENABLED | USB_OPT_MANUAL_PLL)
#else
#define USE_STATIC_OPTIONS \
    (USB_DEVICE_OPT_FULLSPEED | USB_OPT_REG_ENABLED | USB_OPT_AUTO_PLL)
#endif
#define USB_DEVICE_ONLY
#define USE_RAM_DESCRIPTORS
#define FIXED_CONTROL_ENDPOINT_SIZE 64
#define FIXED_NUM_CONFIGURATIONS 1
#define NO_DEVICE_REMOTE_WAKEUP
#define NO_DEVICE_SELF_POWER
#define USE_INTERNAL_SERIAL 3
#define F_USB F_CPU
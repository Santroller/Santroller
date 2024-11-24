#include <stdint.h>
#include "defines.h"
#include "Usb.h"
#include "ids.h"
#include "config.h"
#include "controllers.h"
typedef struct {
    uint8_t console_type;
    uint8_t sub_type;
    uint8_t dev_addr;
    uint8_t instance;
} USB_Device_Type_t;
void get_usb_device_type_for(uint16_t vid, uint16_t pid, uint16_t version, USB_Device_Type_t *type);
inline void fill_device_descriptor(USB_DEVICE_DESCRIPTOR *dev) {
    if (consoleType == SWITCH) {
        dev->idVendor = HORI_VID;
        dev->idProduct = HORI_POKKEN_TOURNAMENT_DX_PRO_PAD_PID;
    } else if (consoleType == XBOX360) {
        dev->idVendor = xbox_360_vid;
        dev->idProduct = xbox_360_pid;
    } else if (consoleType == OG_XBOX) {
        dev->idVendor = MICROSOFT_VID;
        dev->idProduct = DUKE_PID;
    } else if (consoleType == XBOXONE) {
        dev->idVendor = MICROSOFT_VID;
        dev->idProduct = XBOX_ONE_CONTROLLER_PID;
        dev->bDeviceClass = 0xff;
        dev->bDeviceSubClass = 0x47;
        dev->bDeviceProtocol = 0xd0;
    } else if (consoleType == PS4) {
        // We just use the ghl dongle ids for PS4 as PS4 doesn't care
        if (DEVICE_TYPE_IS_GAMEPAD) {
            dev->idVendor = SONY_VID;
            dev->idProduct = PS4_DS_PID_1;
        } else {
            dev->idVendor = XBOX_REDOCTANE_VID;
            dev->idProduct = PS4_GHLIVE_DONGLE_PID;
        }
    } else if (consoleType == IOS_FESTIVAL) {
        dev->idVendor = SONY_VID;
        dev->idProduct = SONY_DS3_PID;
    }
    if (deviceType == GUITAR_HERO_GUITAR) {
        if (consoleType == WII_RB) {
            dev->idVendor = HARMONIX_VID;
            dev->idProduct = WII_RB_GUITAR_PID;
        }
        if (consoleType == PS3) {
            dev->idVendor = REDOCTANE_VID;
            dev->idProduct = PS3_GH_GUITAR_PID;
        }
    }
    if (deviceType == ROCK_BAND_GUITAR) {
        if (consoleType == WII_RB) {
            dev->idVendor = HARMONIX_VID;
            dev->idProduct = WII_RB_GUITAR_PID;
        }
        if (consoleType == PS3) {
            dev->idVendor = REDOCTANE_VID;
            dev->idProduct = PS3_RB_GUITAR_PID;
        }
    }
    if (deviceType == GUITAR_HERO_DRUMS) {
        if (consoleType == WII_RB) {
            dev->idVendor = HARMONIX_VID;
            dev->idProduct = WII_RB_DRUM_PID;
        }
        if (consoleType == PS3) {
            dev->idVendor = REDOCTANE_VID;
            dev->idProduct = PS3_GH_DRUM_PID;
        }
    }
    if (deviceType == ROCK_BAND_DRUMS) {
        if (consoleType == WII_RB) {
            dev->idVendor = HARMONIX_VID;
            dev->idProduct = WII_RB_DRUM_PID;
        }
        if (consoleType == PS3) {
            dev->idVendor = REDOCTANE_VID;
            dev->idProduct = PS3_RB_DRUM_PID;
        }
    }
    if (deviceType == ROCK_BAND_PRO_GUITAR_MUSTANG) {
        if (consoleType == WII_RB) {
            dev->idVendor = HARMONIX_VID;
            dev->idProduct = WII_MUSTANG_PID;
        }
        if (consoleType == PS3) {
            dev->idVendor = REDOCTANE_VID;
            dev->idProduct = PS3_MUSTANG_PID;
        }
    }
    if (deviceType == ROCK_BAND_PRO_GUITAR_SQUIRE) {
        if (consoleType == WII_RB) {
            dev->idVendor = HARMONIX_VID;
            dev->idProduct = WII_SQUIRE_PID;
        }
        if (consoleType == PS3) {
            dev->idVendor = REDOCTANE_VID;
            dev->idProduct = PS3_SQUIRE_PID;
        }
    }
    if (deviceType == ROCK_BAND_PRO_GUITAR_SQUIRE) {
        if (consoleType == WII_RB) {
            dev->idVendor = HARMONIX_VID;
            dev->idProduct = WII_KEYBOARD_PID;
        }
        if (consoleType == PS3) {
            dev->idVendor = REDOCTANE_VID;
            dev->idProduct = PS3_KEYBOARD_PID;
        }
    }
    if (deviceType == LIVE_GUITAR) {
        if (consoleType == PS3) {
            dev->idVendor = REDOCTANE_VID;
            dev->idProduct = PS3WIIU_GHLIVE_DONGLE_PID;
        }
    }
    if (deviceType == DJ_HERO_TURNTABLE) {
        if (consoleType == PS3) {
            dev->idVendor = REDOCTANE_VID;
            dev->idProduct = PS3_DJ_TURNTABLE_PID;
        }
    }
    if (DEVICE_TYPE_IS_GAMEPAD) {
        if (consoleType == PS3) {
            dev->idVendor = SONY_VID;
            dev->idProduct = SONY_DS3_PID;
        }
    }
}

#include <stdint.h>
#include "midi_descriptors.h"
#include "reports/crkd.h"
#ifdef __cplusplus
extern "C" {
#endif
enum SerialCommands {
    COMMAND_REBOOT=0x30,
    COMMAND_JUMP_BOOTLOADER,
    COMMAND_JUMP_BOOTLOADER_UNO,
    COMMAND_JUMP_BOOTLOADER_UNO_USB_THEN_SERIAL,
    COMMAND_READ_CONFIG,
    COMMAND_READ_F_CPU,
    COMMAND_READ_BOARD,
    COMMAND_READ_DIGITAL,
    COMMAND_READ_ANALOG,
    COMMAND_READ_PS2,
    COMMAND_READ_WII,
    COMMAND_READ_DJ_LEFT,
    COMMAND_READ_DJ_RIGHT,
    COMMAND_READ_GH5,
    COMMAND_READ_GHWT,
    COMMAND_GET_EXTENSION_WII,
    COMMAND_GET_EXTENSION_PS2,
    COMMAND_SET_LEDS,
    COMMAND_DISABLE_MULTIPLEXER,
    COMMAND_READ_SERIAL,
    COMMAND_READ_RF,
    COMMAND_READ_USB_HOST,
    COMMAND_START_BT_SCAN,
    COMMAND_STOP_BT_SCAN,
    COMMAND_GET_BT_DEVICES,
    COMMAND_GET_BT_STATE,
    COMMAND_GET_BT_ADDRESS,
    COMMAND_READ_USB_HOST_INPUTS,
    COMMAND_READ_PERIPHERAL_DIGITAL,
    COMMAND_READ_PERIPHERAL_GHWT,
    COMMAND_READ_PERIPHERAL_VALID,
    COMMAND_READ_CLONE,
    COMMAND_SET_LEDS_PERIPHERAL,
    COMMAND_WRITE_ANALOG,
    COMMAND_WRITE_DIGITAL,
    COMMAND_LED_BRIGHTNESS,
    COMMAND_READ_ACCEL,
    COMMAND_READ_MPR121,
    COMMAND_READ_MPR121_VALID,
    COMMAND_SET_LEDS_MPR121,
    COMMAND_READ_MAX170X,
    COMMAND_READ_MAX170X_VALID,
    COMMAND_READ_MIDI,
    COMMAND_SET_ACCEL_FILTER,
    COMMAND_ACCEL_VALID,
    COMMAND_READ_BLUETOOTH_INPUTS,
    COMMAND_WT_DRUM_VALID,
    COMMAND_BH_DRUM_VALID,
    COMMAND_READ_CRKD,
    MAX=100
};

extern uint8_t lastSuccessfulPS2Packet[32];
extern uint8_t lastSuccessfulWiiPacket[8];
extern uint8_t lastSuccessfulTurntablePacketLeft[3];
extern uint8_t lastSuccessfulTurntablePacketRight[3];
extern uint8_t lastSuccessfulGH5Packet[2];
extern uint8_t lastSuccessfulClonePacket[4];
extern crkd_neck_t lastCrkd;
extern uint8_t wiiBytes;
extern uint32_t lastWt[5];
extern uint8_t rawWt;
extern uint8_t rawWtPeripheral;
extern bool lastGH5WasSuccessful;
extern bool lastCloneWasSuccessful;
extern bool lastCrkdWasSuccessful;
extern bool lastTurntableWasSuccessfulLeft;
extern bool lastTurntableWasSuccessfulRight;
extern bool lastWiiWasSuccessful;
extern bool lastPS2WasSuccessful;
extern bool wt_drum_found;
extern bool bh_drum_found;
extern uint16_t wiiControllerType;
extern uint8_t ps2ControllerType;
extern bool overrideR2;
extern bool slave_initted;
extern uint8_t overriddenR2;
extern int16_t filtered[3];
extern uint16_t accel_adc[3];
extern bool accel_found;
extern bool mpr121_init;
extern uint16_t lastMpr121;
extern bool max170x_init;
extern uint8_t lastBattery;
extern Midi_Data_t midiData;
extern double currentLowPassAlpha;
#ifdef __cplusplus
}
#endif
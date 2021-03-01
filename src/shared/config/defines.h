#pragma once
#define NONE 0
#define INVALID_PIN 0xFF
#define REAL_GUITAR_SUBTYPE 7
#define REAL_DRUM_SUBTYPE 8
/** Enum for the device interface descriptor IDs within the device. Each
 * interface descriptor should have a unique ID index associated with it, which
 * can be used to refer to the interface from other descriptors.
 */
enum InterfaceDescriptors_t {
  INTERFACE_ID_HID = 0,    /**< HID interface descriptor ID */
  INTERFACE_ID_XInput = 1, /**< XInput interface descriptor ID */
  INTERFACE_ID_Config = 2, /**< XInput interface descriptor ID */
#ifdef MULTI_ADAPTOR
  INTERFACE_ID_XInput_2 = 3, /**< XInput interface descriptor ID */
  INTERFACE_ID_XInput_3 = 4, /**< XInput interface descriptor ID */
  INTERFACE_ID_XInput_4 = 5, /**< XInput interface descriptor ID */
#else
  INTERFACE_ID_ControlStream =
      3, /**< MIDI Control Stream interface descriptor ID */
  INTERFACE_ID_AudioStream =
      4, /**< MIDI Audio Stream interface descriptor ID */
#endif
};
// Tilt detection
enum TiltType { NO_TILT, MPU_6050, DIGITAL, ANALOGUE };

// Input types
enum InputType { WII = 1, DIRECT, PS2 };

enum SubType {
  XINPUT_GAMEPAD = 1,
  XINPUT_WHEEL,
  XINPUT_ARCADE_STICK,
  XINPUT_FLIGHT_STICK,
  XINPUT_DANCE_PAD,
  XINPUT_LIVE_GUITAR = 9,
  XINPUT_ROCK_BAND_DRUMS = 12,
  XINPUT_GUITAR_HERO_DRUMS,
  XINPUT_ROCK_BAND_GUITAR,
  XINPUT_GUITAR_HERO_GUITAR,
  XINPUT_ARCADE_PAD = 19,
  KEYBOARD_GAMEPAD,
  KEYBOARD_GUITAR_HERO_GUITAR,
  KEYBOARD_ROCK_BAND_GUITAR,
  KEYBOARD_LIVE_GUITAR,
  KEYBOARD_GUITAR_HERO_DRUMS,
  KEYBOARD_ROCK_BAND_DRUMS,
  SWITCH_GAMEPAD,
  PS3_GUITAR_HERO_GUITAR,
  PS3_GUITAR_HERO_DRUMS,
  PS3_ROCK_BAND_GUITAR,
  PS3_ROCK_BAND_DRUMS,
  PS3_GAMEPAD,
  WII_ROCK_BAND_GUITAR,
  WII_ROCK_BAND_DRUMS,
  MOUSE,
  MIDI_GAMEPAD,
  MIDI_GUITAR_HERO_GUITAR,
  MIDI_ROCK_BAND_GUITAR,
  MIDI_LIVE_GUITAR,
  MIDI_GUITAR_HERO_DRUMS,
  MIDI_ROCK_BAND_DRUMS
};

// Orientations for gyros
enum GyroOrientation {
  POSITIVE_Z,
  NEGATIVE_Z,
  POSITIVE_Y,
  NEGATIVE_Y,
  POSITIVE_X,
  NEGATIVE_X
};

// Firmware types
enum FirmwareType {
  GUITAR_DEVICE_TYPE = 0xfea124,
  ARDWIINO_DEVICE_TYPE = 0xa2d415
};

// Fret Modes
enum FretLedMode { LEDS_DISABLED, LEDS_INLINE, APA102 };

enum MidiType { DISABLED, NOTE, CONTROL_COMMAND };

enum PinTypeFlags {
  DIGITAL_PIN,
  ANALOGUE_PIN,
  INVERTED,
};
enum WiiExtType {
  WII_NUNCHUK = 0x0000,
  WII_CLASSIC_CONTROLLER = 0x0001,
  WII_CLASSIC_CONTROLLER_PRO = 0x0101,
  WII_THQ_UDRAW_TABLET = 0xFF12,
  WII_UBISOFT_DRAWSOME_TABLET = 0xFF13,
  WII_GUITAR_HERO_GUITAR_CONTROLLER = 0x0003,
  WII_GUITAR_HERO_DRUM_CONTROLLER = 0x0103,
  WII_DJ_HERO_TURNTABLE = 0x0303,
  WII_TAIKO_NO_TATSUJIN_CONTROLLER = 0x0011,
  WII_MOTION_PLUS = 0x0005,
  WII_NO_EXTENSION = 0x180b,
  WII_NOT_INITIALISED = 0xFFFF
};
enum PsxControllerType {
  PSX_UNKNOWN_CONTROLLER = 0,
  PSX_DUALSHOCK_1_CONTROLLER,
  PSX_DUALSHOCK_2_CONTROLLER,
  PSX_WIRELESS_SONY_DUALSHOCK_CONTROLLER,
  PSX_GUITAR_HERO_CONTROLLER,
  PSX_NEGCON,
  PSX_ANALOG,
  PSX_MOUSE,
  PSX_NO_DEVICE
};
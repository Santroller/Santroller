/**
 * You should have a LUFAConfig.h for this to work.
 */

#include "LUFAConfig.h"
/**
 * Include LUFA.h after LUFAConfig.h
 */
#include <LUFA.h>
#include <LUFA/LUFA/Drivers/Board/LEDs.h>
#include <LUFA/LUFA/Drivers/USB/Class/CDCClass.h>
#include <LUFA/LUFA/Drivers/USB/USB.h>
#include <LUFA/LUFA/Platform/Platform.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/power.h>
#include <avr/wdt.h>
#include <string.h>

#include "commands.h"
#include "config.h"
#include "descriptors.h"
#include "hid.h"
#include "shared_main.h"

typedef struct
{
    USB_Descriptor_Header_t Header;
    uint16_t UnicodeString[(INTERNAL_SERIAL_LENGTH_BITS / 4) + 3];
} __attribute__ ((packed)) SignatureDescriptor_t;
volatile uint16_t persistedConsoleType __attribute__((section(".noinit")));
volatile uint16_t persistedConsoleTypeValid __attribute__((section(".noinit")));
void SetupHardware(void);

bool connected = false;
void EVENT_USB_Device_Connect(void) {
}

void EVENT_USB_Device_Disconnect(void) {
    connected = false;
}

bool usb_configured() {
    return connected;
}

void setup() {
    init_main();
    if (persistedConsoleTypeValid == 0x3A2F) {
        consoleType = persistedConsoleType;
    }
    GlobalInterruptEnable();  // enable global interrupts
    SetupHardware();          // ask LUFA to setup the hardware
}

uint8_t buf[255];
void loop() {
    tick();
    Endpoint_SelectEndpoint(DEVICE_EPADDR_OUT);
    if (Endpoint_IsOUTReceived()) {
        if (Endpoint_IsReadWriteAllowed()) {
            uint8_t size = Endpoint_BytesInEndpoint();
            Endpoint_Read_Stream_LE(buf, size, NULL);
            hid_set_report(buf, size, INTERRUPT_ID, INTERRUPT_ID);
        }
        Endpoint_ClearOUT();
    }
}
bool ready_for_next_packet() {
    Endpoint_SelectEndpoint(DEVICE_EPADDR_IN);
    return Endpoint_IsINReady();
}
void send_report_to_pc(const void* report, uint8_t len) {
    Endpoint_SelectEndpoint(DEVICE_EPADDR_IN);
    Endpoint_Write_Stream_LE(report, len, NULL);
    Endpoint_ClearIN();
}

void SetupHardware(void) {
    /* Disable watchdog if enabled by bootloader/fuses */
    MCUSR &= ~(1 << WDRF);
    wdt_disable();

    /* Disable clock division */
    clock_prescale_set(clock_div_1);

    /* Hardware Initialization */
    USB_Init();
}

void EVENT_USB_Device_ControlRequest(void) {
    if (controlRequestValid(USB_ControlRequest.bmRequestType, USB_ControlRequest.bRequest, USB_ControlRequest.wValue, USB_ControlRequest.wIndex, USB_ControlRequest.wLength)) {
        if ((USB_ControlRequest.bmRequestType & CONTROL_REQTYPE_DIRECTION) == (REQDIR_DEVICETOHOST)) {
            uint16_t len = controlRequest(USB_ControlRequest.bmRequestType, USB_ControlRequest.bRequest, USB_ControlRequest.wValue, USB_ControlRequest.wIndex, USB_ControlRequest.wLength, buf);
            Endpoint_ClearSETUP();
            Endpoint_Write_Control_Stream_LE(buf, len);
            Endpoint_ClearStatusStage();
        } else {
            Endpoint_ClearSETUP();
            Endpoint_Read_Control_Stream_LE(buf, USB_ControlRequest.wLength);
            controlRequest(USB_ControlRequest.bmRequestType, USB_ControlRequest.bRequest, USB_ControlRequest.wValue, USB_ControlRequest.wIndex, USB_ControlRequest.wLength, buf);
            Endpoint_ClearStatusStage();
        }
    }
}
void EVENT_USB_Device_ConfigurationChanged(void) {
    uint8_t type = EP_TYPE_INTERRUPT;
    uint8_t epsize = 0x20;
    uint8_t epsizeOut = 0x08;
    if (consoleType == WINDOWS || consoleType == XBOX360) {
        epsize = 0x18;
    }
    if (consoleType == MIDI) {
        type = EP_TYPE_BULK;
    }
    Endpoint_ConfigureEndpoint(DEVICE_EPADDR_IN, type, epsize, 2);
    Endpoint_ConfigureEndpoint(DEVICE_EPADDR_OUT, type, epsizeOut, 2);
    connected = true;
}

static void USB_Device_GetInternalSerialDescriptor(void) {
    SignatureDescriptor_t* desc = (SignatureDescriptor_t*)buf;
    desc->Header.Type = DTYPE_String;
    desc->Header.Size = USB_STRING_LEN((INTERNAL_SERIAL_LENGTH_BITS / 4) + 3);

    uint_reg_t CurrentGlobalInt = GetGlobalInterruptMask();
    GlobalInterruptDisable();

    uint8_t SigReadAddress = INTERNAL_SERIAL_START_ADDRESS;

    for (uint8_t SerialCharNum = 0; SerialCharNum < (INTERNAL_SERIAL_LENGTH_BITS / 4); SerialCharNum++) {
        uint8_t SerialByte = boot_signature_byte_get(SigReadAddress);

        if (SerialCharNum & 0x01) {
            SerialByte >>= 4;
            SigReadAddress++;
        }

        SerialByte &= 0x0F;

        desc->UnicodeString[SerialCharNum] = cpu_to_le16((SerialByte >= 10) ? (('A' - 10) + SerialByte) : ('0' + SerialByte));
    }

    SetGlobalInterruptMask(CurrentGlobalInt);
    desc->UnicodeString[(INTERNAL_SERIAL_LENGTH_BITS / 4)] = consoleType + '0';
    desc->UnicodeString[(INTERNAL_SERIAL_LENGTH_BITS / 4)+1] = DEVICE_TYPE + '0';
    #if DEVICE_TYPE_IS_GAMEPAD
    desc->UnicodeString[(INTERNAL_SERIAL_LENGTH_BITS / 4)+2] = WINDOWS_USES_XINPUT + '0';
    #else
    desc->UnicodeString[(INTERNAL_SERIAL_LENGTH_BITS / 4)+2] = '0';
    #endif
}

uint16_t CALLBACK_USB_GetDescriptor(const uint16_t wValue,
                                    const uint16_t wIndex,
                                    const void** const descriptorAddress) {
    *descriptorAddress = buf;
    if (wValue == ((DTYPE_String << 8) | 3)) {
        USB_Device_GetInternalSerialDescriptor();
        return sizeof(SignatureDescriptor_t);
    }
    return descriptorRequest(wValue, wIndex, buf);
}
void reset_usb(void) {
    persistedConsoleType = consoleType;
    persistedConsoleTypeValid = 0x3A2F;
    reboot();
}
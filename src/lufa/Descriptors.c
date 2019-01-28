/*
     .k8GOGGNqkSFS5XkqXPSkSkkqXXFS5kSkSS15U22F2515U2uujuu1U1u2U1U2uUuFS.
   :0qE     JS5uuJuuFFX51jU2SSk12jU2SSXF5uuu15SFS5k12ujj21S5kFS5S12jJYu11
  5XS:        1UYYLu.   vUUX    U22r     SUF         SUF           ;YYLuU5
 1F5i  NNSkS7  2uLJui   51u     S5.      .PX         .XX           LJvLLu1.
 kUk  0iLk5FFu vuYY2:   5F    Xkk7        78    E0    i0    GEXPXk2uLLvLLuk
X25, 8O   2kX0  5YJUi   M    555    PkXk   i    q1FU   7    ONNkP12YLvLvLYS
S25  8888  888  5uY5         FuS    PS50   .    FuUU   7          uJvLvLLJ2i
kUF             SJjU.      P02UF    P25k   .    Su2Y   v          2LLvLvLL17
S21  XJj88  0u  1uY2.        X2k           .    k11E   v    7;ii:JuJvLvLvJ2:
2257 jqv   Pqq  1LJur         PP.          7    EX:    q    OqqXP51JYvLvYYS.
 X2F  kXkXSXk  kJYLU:   O     ,Z    0PXZ   i    ii    q0    i:::,,.jLLvLLuF'
 ik1k  ;qkPj  .uJvYu:   UN      :   XU2F   :         S5S           iJLLvjUF8
  :PSq       72uLLLui   uSi    .;   2uY1   r.       72j1           LYYLYJSU88
    XqE2   rP12juJuu1FX55U5FqXXSXkXF1juUkkPSXSPXPXPF1Jju5FkFSFXFSF5uujUu5j28V
      .uGOZESS5S5SFkkPkPkXkPXqXPXqXXFkSkkPXPXPkqSkSS1521252121U2u2u12Suv7

*
* Arduino Micro (Leonardo) XInput Pad Emulator firmware
*
* Copyright (c) 2017
* Bruno Freitas - bruno@brunofreitas.com
* Jon Wilson    - degenatrons@gmail.com
* Kevin Mackett - kevin@sharpfork.com
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

/** \file
 *
 *  USB Device Descriptors, for library use when in USB device mode. Descriptors are special
 *  computer-readable structures which the host requests upon device enumeration, to determine
 *  the device's capabilities and functions.
 */

#include "Descriptors.h"

/** Device descriptor structure. This descriptor, located in FLASH memory, describes the overall
 *  device characteristics, including the supported USB version, control endpoint size and the
 *  number of device configurations. The descriptor is read out by the USB host when the enumeration
 *  process begins.
 */
const uint8_t PROGMEM DeviceDescriptor[] =
{
	0x12,        // bLength
	0x01,        // bDescriptorType (Device)
	0x00, 0x02,  // bcdUSB 2.00
	0xFF,        // bDeviceClass 
	0xFF,        // bDeviceSubClass 
	0xFF,        // bDeviceProtocol 
	0x40,        // bMaxPacketSize0 64
	0x5E, 0x04,  // idVendor 0x045E
	0x8E, 0x02,  // idProduct 0x028E
	0x14, 0x01,  // bcdDevice 2.14
	0x01,        // iManufacturer (String Index)
	0x02,        // iProduct (String Index)
	0x03,        // iSerialNumber (String Index)
	0x01        // bNumConfigurations 1
};

/** Configuration descriptor structure. This descriptor, located in FLASH memory, describes the usage
 *  of the device in one of its supported configurations, including information about any device interfaces
 *  and endpoints. The descriptor is read out by the USB host during the enumeration process when selecting
 *  a configuration so that the host may correctly communicate with the USB device.
 */
const uint8_t PROGMEM ConfigurationDescriptor[] =
{
	0x09,        // bLength
	0x02,        // bDescriptorType (Configuration)
	0x99, 0x00,  // wTotalLength 153
	0x04,        // bNumInterfaces 4
	0x01,        // bConfigurationValue
	0x00,        // iConfiguration (String Index)
	0xA0,        // bmAttributes Remote Wakeup
	0xFA,        // bMaxPower 500mA
	
	0x09,        // bLength
	0x04,        // bDescriptorType (Interface)
	0x00,        // bInterfaceNumber 0
	0x00,        // bAlternateSetting
	0x02,        // bNumEndpoints 2
	0xFF,        // bInterfaceClass
	0x5D,        // bInterfaceSubClass
	0x01,        // bInterfaceProtocol
	0x00,        // iInterface (String Index)
	
	0x11,        // bLength
	0x21,        // bDescriptorType (HID)
	0x00, 0x01,  // bcdHID 1.00
	0x01,        // bCountryCode
	0x25,        // bNumDescriptors
	0x81,        // bDescriptorType[0] (Unknown 0x81)
	0x14, 0x00,  // wDescriptorLength[0] 20
	0x00,        // bDescriptorType[1] (Unknown 0x00)
	0x00, 0x00,  // wDescriptorLength[1] 0
	0x13,        // bDescriptorType[2] (Unknown 0x13)
	0x01, 0x08,  // wDescriptorLength[2] 2049
	0x00,        // bDescriptorType[3] (Unknown 0x00)
	0x00, 
	0x07,        // bLength
	0x05,        // bDescriptorType (Endpoint)
	0x81,        // bEndpointAddress (IN/D2H)
	0x03,        // bmAttributes (Interrupt)
	0x20, 0x00,  // wMaxPacketSize 32
	0x01,        // bInterval 4 (unit depends on device speed)
	
	0x07,        // bLength
	0x05,        // bDescriptorType (Endpoint)
	0x01,        // bEndpointAddress (OUT/H2D)
	0x03,        // bmAttributes (Interrupt)
	0x20, 0x00,  // wMaxPacketSize 32
	0x01,        // bInterval 8 (unit depends on device speed)
	
	0x09,        // bLength
	0x04,        // bDescriptorType (Interface)
	0x01,        // bInterfaceNumber 1
	0x00,        // bAlternateSetting
	0x04,        // bNumEndpoints 4
	0xFF,        // bInterfaceClass
	0x5D,        // bInterfaceSubClass
	0x03,        // bInterfaceProtocol
	0x00,        // iInterface (String Index)
	
	0x1B,        // bLength
	0x21,        // bDescriptorType (HID)
	0x00, 0x01,  // bcdHID 1.00
	0x01,        // bCountryCode
	0x01,        // bNumDescriptors
	0x82,        // bDescriptorType[0] (Unknown 0x82)
	0x40, 0x01,  // wDescriptorLength[0] 320
	0x02, 0x20, 0x16, 0x83, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x16, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x07,        // bLength
	0x05,        // bDescriptorType (Endpoint)
	0x82,        // bEndpointAddress (IN/D2H)
	0x03,        // bmAttributes (Interrupt)
	0x20, 0x00,  // wMaxPacketSize 32
	0x01,        // bInterval 2 (unit depends on device speed)
	
	0x07,        // bLength
	0x05,        // bDescriptorType (Endpoint)
	0x02,        // bEndpointAddress (OUT/H2D)
	0x03,        // bmAttributes (Interrupt)
	0x20, 0x00,  // wMaxPacketSize 32
	0x01,        // bInterval 4 (unit depends on device speed)
	
	0x07,        // bLength
	0x05,        // bDescriptorType (Endpoint)
	0x83,        // bEndpointAddress (IN/D2H)
	0x03,        // bmAttributes (Interrupt)
	0x20, 0x00,  // wMaxPacketSize 32
	0x1,        // bInterval 64 (unit depends on device speed)
	
	0x07,        // bLength
	0x05,        // bDescriptorType (Endpoint)
	0x03,        // bEndpointAddress (OUT/H2D)
	0x03,        // bmAttributes (Interrupt)
	0x20, 0x00,  // wMaxPacketSize 32
	0x1,        // bInterval 16 (unit depends on device speed)
	
	0x09,        // bLength
	0x04,        // bDescriptorType (Interface)
	0x02,        // bInterfaceNumber 2
	0x00,        // bAlternateSetting
	0x01,        // bNumEndpoints 1
	0xFF,        // bInterfaceClass
	0x5D,        // bInterfaceSubClass
	0x02,        // bInterfaceProtocol
	0x00,        // iInterface (String Index)
	
	0x09,        // bLength
	0x21,        // bDescriptorType (HID)
	0x00, 0x01,  // bcdHID 1.00
	0x01,        // bCountryCode
	0x22,        // bNumDescriptors
	0x84,        // bDescriptorType[0] (Unknown 0x84)
	0x07, 0x00,  // wDescriptorLength[0] 7
	
	0x07,        // bLength
	0x05,        // bDescriptorType (Endpoint)
	0x84,        // bEndpointAddress (IN/D2H)
	0x03,        // bmAttributes (Interrupt)
	0x20, 0x00,  // wMaxPacketSize 32
	0x1,        // bInterval 16 (unit depends on device speed)
	
	0x09,        // bLength
	0x04,        // bDescriptorType (Interface)
	0x03,        // bInterfaceNumber 3
	0x00,        // bAlternateSetting
	0x00,        // bNumEndpoints 0
	0xFF,        // bInterfaceClass
	0xFD,        // bInterfaceSubClass
	0x13,        // bInterfaceProtocol
	0x04,        // iInterface (String Index)
	
	0x06,        // bLength
	0x41,        // bDescriptorType (Unknown)
	0x00, 0x01, 0x01, 0x03
};

/** Language descriptor structure. This descriptor, located in FLASH memory, is returned when the host requests
 *  the string descriptor with index 0 (the first index). It is actually an array of 16-bit integers, which indicate
 *  via the language ID table available at USB.org what languages the device supports for its string descriptors.
 */
const USB_Descriptor_String_t PROGMEM LanguageString =
{
	.Header                 = {.Size = USB_STRING_LEN(1), .Type = DTYPE_String},

	.UnicodeString          = {LANGUAGE_ID_ENG}
};

/** Manufacturer descriptor string. This is a Unicode string containing the manufacturer's details in human readable
 *  form, and is read out upon request by the host when the appropriate string ID is requested, listed in the Device
 *  Descriptor.
 */
const USB_Descriptor_String_t PROGMEM ManufacturerString =
{
	.Header                 = {.Size = USB_STRING_LEN(12), .Type = DTYPE_String},

	.UnicodeString          = L"sanjay900"
};

/** Product descriptor string. This is a Unicode string containing the product's details in human readable form,
 *  and is read out upon request by the host when the appropriate string ID is requested, listed in the Device
 *  Descriptor.
 */
const USB_Descriptor_String_t PROGMEM ProductString =
{
	.Header                 = {.Size = USB_STRING_LEN(31), .Type = DTYPE_String},

	.UnicodeString          = L"Wii Controller Xbox Adaptor"
};

const USB_Descriptor_String_t PROGMEM VersionString =
{
	.Header                 = {.Size = USB_STRING_LEN(3), .Type = DTYPE_String},

	.UnicodeString          = L"1.0"
};

/** This function is called by the library when in device mode, and must be overridden (see library "USB Descriptors"
 *  documentation) by the application code so that the address and size of a requested descriptor can be given
 *  to the USB library. When the device receives a Get Descriptor request on the control endpoint, this function
 *  is called so that the descriptor details can be passed back and the appropriate descriptor sent back to the
 *  USB host.
 */
uint16_t CALLBACK_USB_GetDescriptor(const uint16_t wValue,
                                    const uint16_t wIndex,
                                    const void** const DescriptorAddress)
{
	const uint8_t  DescriptorType   = (wValue >> 8);
	const uint8_t  DescriptorNumber = (wValue & 0xFF);

	const void* Address = NULL;
	uint16_t    Size    = NO_DESCRIPTOR;

	switch (DescriptorType)
	{
		case DTYPE_Device:
			Address = &DeviceDescriptor;
			Size    = sizeof(DeviceDescriptor);
			break;
		case DTYPE_Configuration:
			Address = &ConfigurationDescriptor;
			Size    = sizeof(ConfigurationDescriptor);
			break;
		case DTYPE_String:
			switch (DescriptorNumber)
			{
				case 0x00:
					Address = &LanguageString;
					Size    = pgm_read_byte(&LanguageString.Header.Size);
					break;
				case 0x01:
					Address = &ManufacturerString;
					Size    = pgm_read_byte(&ManufacturerString.Header.Size);
					break;
				case 0x02:
					Address = &ProductString;
					Size    = pgm_read_byte(&ProductString.Header.Size);
					break;
				case 0x03:
					Address = &VersionString;
					Size    = pgm_read_byte(&VersionString.Header.Size);
					break;
		}

			break;
	}

	*DescriptorAddress = Address;
	return Size;
}


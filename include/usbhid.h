/* Copyright (C) 2011 Circuits At Home, LTD. All rights reserved.

This software may be distributed and modified under the terms of the GNU
General Public License version 2 (GPL2) as published by the Free Software
Foundation and appearing in the file GPL2.TXT included in the packaging of
this file. Please note that GPL2 Section 2[b] requires that all works based
on this software must also be made publicly available under the terms of
the GPL2 ("Copyleft").

Contact information
-------------------

Circuits At Home, LTD
Web      :  http://www.circuitsathome.com
e-mail   :  support@circuitsathome.com
 */
#pragma once

#include "Usb.h"

#define MAX_REPORT_PARSERS                      2
#define HID_MAX_HID_CLASS_DESCRIPTORS           5

#define DATA_SIZE_MASK                          0x03
#define TYPE_MASK                               0x0C
#define TAG_MASK                                0xF0

#define DATA_SIZE_0                             0x00
#define DATA_SIZE_1                             0x01
#define DATA_SIZE_2                             0x02
#define DATA_SIZE_4                             0x03

#define TYPE_MAIN                               0x00
#define TYPE_GLOBAL                             0x04
#define TYPE_LOCAL                              0x08

#define TAG_MAIN_INPUT                          0x80
#define TAG_MAIN_OUTPUT                         0x90
#define TAG_MAIN_COLLECTION                     0xA0
#define TAG_MAIN_FEATURE                        0xB0
#define TAG_MAIN_ENDCOLLECTION                  0xC0

#define TAG_GLOBAL_USAGEPAGE                    0x00
#define TAG_GLOBAL_LOGICALMIN                   0x10
#define TAG_GLOBAL_LOGICALMAX                   0x20
#define TAG_GLOBAL_PHYSMIN                      0x30
#define TAG_GLOBAL_PHYSMAX                      0x40
#define TAG_GLOBAL_UNITEXP                      0x50
#define TAG_GLOBAL_UNIT                         0x60
#define TAG_GLOBAL_REPORTSIZE                   0x70
#define TAG_GLOBAL_REPORTID                     0x80
#define TAG_GLOBAL_REPORTCOUNT                  0x90
#define TAG_GLOBAL_PUSH                         0xA0
#define TAG_GLOBAL_POP                          0xB0

#define TAG_LOCAL_USAGE                         0x00
#define TAG_LOCAL_USAGEMIN                      0x10
#define TAG_LOCAL_USAGEMAX                      0x20

/* HID requests */
#define bmREQ_HID_OUT                           USB_SETUP_HOST_TO_DEVICE|USB_SETUP_TYPE_CLASS|USB_SETUP_RECIPIENT_INTERFACE
#define bmREQ_HID_IN                            USB_SETUP_DEVICE_TO_HOST|USB_SETUP_TYPE_CLASS|USB_SETUP_RECIPIENT_INTERFACE
#define bmREQ_HID_REPORT                        USB_SETUP_DEVICE_TO_HOST|USB_SETUP_TYPE_STANDARD|USB_SETUP_RECIPIENT_INTERFACE

/* HID constants. Not part of chapter 9 */
/* Class-Specific Requests */
#define HID_REQUEST_GET_REPORT                  0x01
#define HID_REQUEST_GET_IDLE                    0x02
#define HID_REQUEST_GET_PROTOCOL                0x03
#define HID_REQUEST_SET_REPORT                  0x09
#define HID_REQUEST_SET_IDLE                    0x0A
#define HID_REQUEST_SET_PROTOCOL                0x0B

/* Class Descriptor Types */
#define HID_DESCRIPTOR_HID                      0x21
#define HID_DESCRIPTOR_REPORT                   0x22
#define HID_DESRIPTOR_PHY                       0x23

/* Protocol Selection */
#define USB_HID_BOOT_PROTOCOL                   0x00
#define HID_RPT_PROTOCOL                        0x01

/* HID Interface Class Code */
#define HID_INTF                                0x03

/* HID Interface Class SubClass Codes */
#define HID_BOOT_INTF_SUBCLASS                  0x01

/* HID Interface Class Protocol Codes */
#define USB_HID_PROTOCOL_NONE                       0x00
#define USB_HID_PROTOCOL_KEYBOARD                   0x01
#define USB_HID_PROTOCOL_MOUSE                      0x02

#define HID_ITEM_TYPE_MAIN                      0
#define HID_ITEM_TYPE_GLOBAL                    1
#define HID_ITEM_TYPE_LOCAL                     2
#define HID_ITEM_TYPE_RESERVED                  3

#define HID_LONG_ITEM_PREFIX                    0xfe    // Long item prefix value

#define bmHID_MAIN_ITEM_TAG                     0xfc    // Main item tag mask

#define bmHID_MAIN_ITEM_INPUT                   0x80    // Main item Input tag value
#define bmHID_MAIN_ITEM_OUTPUT                  0x90    // Main item Output tag value
#define bmHID_MAIN_ITEM_FEATURE                 0xb0    // Main item Feature tag value
#define bmHID_MAIN_ITEM_COLLECTION              0xa0    // Main item Collection tag value
#define bmHID_MAIN_ITEM_END_COLLECTION          0xce    // Main item End Collection tag value

#define HID_MAIN_ITEM_COLLECTION_PHYSICAL       0
#define HID_MAIN_ITEM_COLLECTION_APPLICATION    1
#define HID_MAIN_ITEM_COLLECTION_LOGICAL        2
#define HID_MAIN_ITEM_COLLECTION_REPORT         3
#define HID_MAIN_ITEM_COLLECTION_NAMED_ARRAY    4
#define HID_MAIN_ITEM_COLLECTION_USAGE_SWITCH   5
#define HID_MAIN_ITEM_COLLECTION_USAGE_MODIFIER 6

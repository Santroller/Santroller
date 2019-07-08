#!/usr/bin/env python3

import usb.core
import sys


class find_class(object):
    def __call__(self, device):
        # first, let's check the device
        if device.idVendor == 0x1209 and device.idProduct == 0x2882:
            return True
        if device.idVendor == 0x0F0D and device.idProduct == 0x0092:
            return True
        if device.idVendor == 0x12ba and device.idProduct == 0x0100:
            return True
        if device.idVendor == 0x12ba and device.idProduct == 0x0200:
            return True
        if device.idVendor == 0x12ba and device.idProduct == 0x0120:
            return True
        if device.idVendor == 0x12ba and device.idProduct == 0x0210:
            return True
        return False


# Make this also handle ps3 guitars
try:
    dev = usb.core.find(custom_match=find_class())
    if dev is None:
        raise ValueError('Device not found')
    else:
        dev.ctrl_transfer(0x00, 0x30, 0x00, 0x00)
except:
    pass

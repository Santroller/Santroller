#!/usr/bin/env python3

import usb.core
import sys

try:
    dev = usb.core.find(idVendor=0x1209, idProduct=0x2882)
    val = 0x30
    if len(sys.argv) > 1 and sys.argv[1] == "serial":
        val = 0x31

    print(hex(val))
    if dev is None:
        raise ValueError('Device not found')
    else:
            dev.ctrl_transfer(0x00, val, 0x00, 0x00)
except:
    pass

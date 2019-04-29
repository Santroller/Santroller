#!/usr/bin/env python3

import usb.core

dev = usb.core.find(idVendor=0x1209, idProduct=0x2882)

if dev is None:
    raise ValueError('Device not found')
else:
    try:
        dev.ctrl_transfer(0x30, 0x30, 0x30, 0x30) 
    except:
        pass


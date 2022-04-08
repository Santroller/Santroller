#!/usr/bin/env python3
# import hid
# h = hid.device()
# h.open(0x1209, 0x2882)
# h.send_feature_report([0x00,52])

import subprocess
import re
import sys
try:
    import usb.core
    import usb.util
except ImportError:
    subprocess.check_call([sys.executable, "-m", "pip", "install", "pyusb"])

import usb.core
import usb.util
from platformio.util import get_serial_ports
REBOOT=48
BOOTLOADER=49
BOOTLOADER_SERIAL=50

def launch_dfu():
    dev = usb.core.find(idVendor=0x03eb)
    dev.ctrl_transfer(0xA1, 3, 0, 0, 8)
    command = [0x04, 0x03, 0x00]
    dev.ctrl_transfer(0x21, 1, 0, 0, command)


Import("env")
def before_upload(source, target, env):
    idVendor = 0x03eb
    idProduct = None
    b_request = BOOTLOADER
    if "/arduino_uno/" in str(source[0]):
        b_request = BOOTLOADER_SERIAL
        idVendor = 0x1209
        idProduct = 0x2883
    print(b_request)
    # find our device
    dev = usb.core.find(idVendor=0x1209, idProduct=0x2882)
    try:
        dev.detach_kernel_driver(0)
    except:
        pass
    try:
        dev.ctrl_transfer(0x21, b_request, 0x00,0x00,[])
    except:
        pass
    
    args = {"idVendor": idVendor}
    if idProduct:
        args["idProduct"] = idProduct
    
    before_ports = get_serial_ports()
    while not usb.core.find(**args):
        pass
    if idProduct == 0x2883:
        env.Replace(UPLOAD_PORT=env.WaitForNewSerialPort(before_ports))

def post_upload(source, target, env):
    if "/arduino_uno/" in str(source[0]):
        env.TouchSerialPort("$UPLOAD_PORT", 2400)
    if "/arduino_uno_mega_usb" in str(source[0]):
        launch_dfu()

env.AddPreAction("upload", before_upload)
env.AddPostAction("upload", post_upload)
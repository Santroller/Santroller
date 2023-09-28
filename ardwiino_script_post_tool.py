#!/usr/bin/env python3
from os.path import join
from pprint import pp
import subprocess
import sys
import re
import traceback
from platformio.util import get_serial_ports
import libusb_package

import usb.core
import usb.util
import os
import psutil

REBOOT = 48
BOOTLOADER = 49
BOOTLOADER_SERIAL = 50

Import("env")

me = psutil.Process(os.getpid())

class Context:
    def __init__(self):
        self.meta = ""

def launch_dfu():
    dev = libusb_package.find(idVendor=0x03eb)
    dev.ctrl_transfer(0xA1, 3, 0, 0, 8)
    command = [0x04, 0x03, 0x00]
    dev.ctrl_transfer(0x21, 1, 0, 0, command)
def launch_dfu_no_reset():
    dev = libusb_package.find(idVendor=0x03eb)
    dev.ctrl_transfer(0xA1, 3, 0, 0, 8)
    command = [0x04, 0x03, 0x01, 0x00, 0x00]
    dev.ctrl_transfer(0x21, 1, 0, 0, command)
    # Since the device disconnects after this, it is expected this request will fail
    try:
        dev.ctrl_transfer(0x21, 1, 0, 0)
    except:
        pass

def post_upload(source, target, env):
    if "arduino_uno_usb" in str(source[0]) or "arduino_mega_2560_usb" in str(source[0]) or "arduino_mega_adk_usb" in str(source[0]):
        before_ports = get_serial_ports()
        print("searching for uno")
        launch_dfu_no_reset()
        new_env = None
        while not new_env:
            if me.parent is None:
                exit(1)
            dev = libusb_package.find(idVendor=0x03eb, idProduct=0x0001)
            if dev:
                new_env = "arduino_uno"
                break
            dev = libusb_package.find(idVendor=0x03eb, idProduct=0x0010)
            if dev:
                new_env = "arduino_mega_2560"
                break
            dev = libusb_package.find(idVendor=0x03eb, idProduct=0x003f)
            if dev:
                new_env = "arduino_mega_adk"
                break
        cwd = os.getcwd()
        os.chdir(env["PROJECT_DIR"])
        env.Replace(UPLOAD_PORT=env.WaitForNewSerialPort(before_ports))
        port = env.subst("$UPLOAD_PORT")
        print(f"Calling {new_env} ({port})")
        subprocess.run([sys.executable,"-m","platformio", "run", "--target", "upload", "--environment", new_env, "--upload-port", port], stderr=subprocess.STDOUT)
        os.chdir(cwd)
        # env.TouchSerialPort("$UPLOAD_PORT", 2400)


env.AddPostAction("upload", post_upload)

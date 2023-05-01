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

REBOOT = 48
BOOTLOADER = 49
BOOTLOADER_SERIAL = 50

Import("env")


class Context:
    def __init__(self):
        self.meta = ""


def post_upload(source, target, env):
    if "arduino_uno_usb" in str(source[0]) or "arduino_mega_2560_usb" in str(source[0]) or "arduino_mega_adk_usb" in str(source[0]):
        before_ports = get_serial_ports()
        print("searching for uno")
        new_env = None
        while not new_env:
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
            dev = libusb_package.find(idVendor=0x1209, idProduct=0x2882)
            if not dev:
                dev = libusb_package.find(idVendor=0x1209, idProduct=0x2884)
            if dev:
                try:
                    dev.detach_kernel_driver(0)
                except:
                    pass
                try:
                    dev.ctrl_transfer(0x21, BOOTLOADER_SERIAL)
                    dev.ctrl_transfer(0x21, 0x09, BOOTLOADER_SERIAL)
                except Exception as e:
                    print(e)
                    pass
        cwd = os.getcwd()
        os.chdir(env["PROJECT_DIR"])
        env.Replace(UPLOAD_PORT=env.WaitForNewSerialPort(before_ports))
        port = env.subst("$UPLOAD_PORT")
        print(f"Calling {new_env} ({port})")
        subprocess.run([sys.executable,"-m","platformio", "run", "--target", "upload", "--environment", new_env, "--upload-port", port], stderr=subprocess.STDOUT)
        os.chdir(cwd)
        env.TouchSerialPort("$UPLOAD_PORT", 2400)


env.AddPostAction("upload", post_upload)

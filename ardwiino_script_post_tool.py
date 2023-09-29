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
import time

REBOOT = 48
BOOTLOADER = 49
BOOTLOADER_SERIAL = 50

Import("env")

me = psutil.Process(os.getpid())

class Context:
    def __init__(self):
        self.meta = ""

def post_upload(source, target, env):
    if env["PIOENV"] in ("arduino_uno", "arduino_mega_2560", "arduino_mega_adk") and env["ENV"].get("PROCEED_WITH_USB",""):
        print("Looking for device in DFU mode")
        env.TouchSerialPort("$UPLOAD_PORT", 1200)
        while True:
            if me.parent is None:
                exit(1)
            dev = libusb_package.find(idVendor=0x03eb, idProduct=0x2fef)
            if dev:
                break
            dev = libusb_package.find(idVendor=0x03eb, idProduct=0x2ff7)
            if dev:
                break
        time.sleep(1)
        new_env = f'{env["PIOENV"]}_usb'
        print(f"Calling {new_env}")
        subprocess.run([sys.executable,"-m","platformio", "run", "--target", "upload", "--environment", new_env], stderr=subprocess.STDOUT)
    if env["PIOENV"] in ("arduino_uno_usb", "arduino_mega_2560_usb", "arduino_mega_adk_usb") and env["ENV"].get("PROCEED_WITH_SERIAL",""):
        print("searching for uno")
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
        env["UPLOAD_PORT"] = None
        env.AutodetectUploadPort()
        port = env.subst("$UPLOAD_PORT")
        print(f"Calling {new_env} ({port})")
        subprocess.run([sys.executable,"-m","platformio", "run", "--target", "upload", "--environment", new_env, "--upload-port", port], stderr=subprocess.STDOUT)
        os.chdir(cwd)
        env.TouchSerialPort("$UPLOAD_PORT", 2400)


env.AddPostAction("upload", post_upload)

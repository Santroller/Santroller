#!/usr/bin/env python3
from os.path import join
from pprint import pp
import subprocess
import sys
import re
import traceback

try:
    import usb
except ImportError:
    subprocess.check_call([sys.executable, "-m", "pip", "install", "pyusb"])
try:
    import libusb_package
except ImportError:
    subprocess.check_call([sys.executable, "-m", "pip", "install", "libusb-package"])

import libusb_package

import usb.core
import usb.util
import os

REBOOT = 48
BOOTLOADER = 49
BOOTLOADER_SERIAL = 50


def launch_dfu():
    dev = libusb_package.find(idVendor=0x03eb)
    dev.ctrl_transfer(0xA1, 3, 0, 0, 8)
    command = [0x04, 0x03, 0x00]
    dev.ctrl_transfer(0x21, 1, 0, 0, command)


def launch_dfu_no_reset(dev):
    dev = libusb_package.find(idVendor=0x03eb)
    dev.ctrl_transfer(0xA1, 3, 0, 0, 8)
    command = [0x04, 0x03, 0x01, 0x00, 0x00]
    dev.ctrl_transfer(0x21, 1, 0, 0, command)
    dev.ctrl_transfer(0x21, 1, 0, 0)


Import("env")


class Context:
    def __init__(self):
        self.meta = ""


def post_upload(source, target, env):
    if "/arduino_uno/" in str(source[0]):
        env.TouchSerialPort("$UPLOAD_PORT", 2400)
    if "_usb" in str(source[0]):
        launch_dfu()
        print("searching for uno")
        new_env = None
        while not new_env:
            dev = libusb_package.find(idVendor=0x03eb, idProduct=0x2FF7)
            if dev:
                launch_dfu_no_reset(dev)
            dev = libusb_package.find(idVendor=0x03eb, idProduct=0x2FEF)
            if dev:
                launch_dfu_no_reset(dev)
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
        executable = join(os.getenv("PLATFORMIO_CORE_DIR"), "penv", "bin", "platformio")
        print(f"Calling {new_env}")
        subprocess.run([executable, "run", "--target", "upload", "--environment", new_env], stderr=subprocess.STDOUT)
        os.chdir(cwd)


env.AddPostAction("upload", post_upload)

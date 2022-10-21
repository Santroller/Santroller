#!/usr/bin/env python3
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
from platformio import fs
from platformio.util import get_serial_ports
from platformio.project.config import ProjectConfig
from platformio.run.processor import EnvironmentProcessor
from platformio.run.helpers import clean_build_dir
import os
REBOOT=48
BOOTLOADER=49
BOOTLOADER_SERIAL=50

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
        dev = None
        while not dev:

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
                except:
                    pass
        project_dir = env["PROJECT_DIR"]
        with fs.cd(project_dir):
            config = ProjectConfig.get_instance(
                os.path.join(project_dir, "platformio.ini")
            )
        config.validate()
        processor = EnvironmentProcessor(Context(), new_env,config,["upload"],"",1,"",False, False)
        processor.process()


env.AddPostAction("upload", post_upload)
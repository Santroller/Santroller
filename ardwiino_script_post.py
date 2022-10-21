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
Import("env")
def before_upload(source, target, env):
    upload_options = env.BoardConfig().get("upload", {})
    b_request = None
    exists = False
    id_vendor = None
    id_product = None
    wait_for_serial = False
    if "ardwiino_bootloader_teensy" in upload_options and upload_options["ardwiino_bootloader_teensy"] == "true":
        b_request = BOOTLOADER
    elif "ardwiino_bootloader" in upload_options and upload_options["ardwiino_bootloader"] == "true":
        b_request = BOOTLOADER
        wait_for_serial = True
    if "/arduino_uno_mega_usb" in str(source[0]):
        id_vendor = 0x03eb
        id_product = None
        b_request = BOOTLOADER
        if (libusb_package.find(idVendor=0x1209, idProduct=0x2883)):
            env.AutodetectUploadPort()
            env.TouchSerialPort("$UPLOAD_PORT", 1200)
    if "/arduino_uno/" in str(source[0]):
        if libusb_package.find(idVendor=0x1209, idProduct=0x2882):
            b_request = BOOTLOADER_SERIAL
            id_vendor = 0x1209
            id_product = 0x2883
            exists = libusb_package.find(idProduct=id_product, idVendor=id_vendor)
    before_ports = get_serial_ports()
    if b_request:
        # find our device
        dev = libusb_package.find(idVendor=0x1209, idProduct=0x2882)
        try:
            dev.detach_kernel_driver(0)
        except:
            pass
        try:
            dev.ctrl_transfer(0x21, b_request)
            dev.ctrl_transfer(0x21, 0x09, b_request)
        except:
            pass
    if id_vendor:
        args = {"idVendor": id_vendor}
        if id_product:
            args["idProduct"] = id_product
        
        while not libusb_package.find(**args):
            pass
        if not exists and id_product == 0x2883:
            wait_for_serial = True
    if wait_for_serial:
        env.Replace(UPLOAD_PORT=env.WaitForNewSerialPort(before_ports))

def post_upload(source, target, env):
    if "/arduino_uno/" in str(source[0]):
        env.TouchSerialPort("$UPLOAD_PORT", 2400)
    if "_usb" in str(source[0]):
        launch_dfu_no_reset()


env.AddPreAction("upload", before_upload)
env.AddPostAction("upload", post_upload)
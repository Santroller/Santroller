#!/usr/bin/env python3
from pprint import pp
import subprocess
import sys
from os.path import join
try:
    import libusb_package
except ImportError:
    subprocess.check_call([sys.executable, "-m", "pip", "install", "libusb-package"])

import libusb_package
from serial import Serial, SerialException
import os
from time import sleep

class Context:
    def __init__(self):
        self.meta = ""

Import("env")
if "upload" in BUILD_TARGETS:
    upload_options = env.BoardConfig().get("upload", {})
    if "detect_controller" in upload_options and upload_options["detect_controller"] == "true":
        if (libusb_package.find(idVendor=0x03eb,idProduct=0x0010) or libusb_package.find(idVendor=0x03eb,idProduct=0x003f) or libusb_package.find(idVendor=0x03eb,idProduct=0x0001)):
            print("Found Uno/Mega, touching port")
            env.AutodetectUploadPort()
            env.TouchSerialPort("$UPLOAD_PORT", 1200)
        print("Detecting microcontroller type")
        dev = None
        while not dev:
            dev = libusb_package.find(idVendor=0x03eb, idProduct=0x2FF7)
            if dev:
                env["BOARD_MCU"] = "at90usb82"
                break
            dev = libusb_package.find(idVendor=0x03eb, idProduct=0x2FEF)
            if dev:
                env["BOARD_MCU"] = "atmega16u2"
                break
            pass
    if "detect_frequency" in upload_options and upload_options["detect_frequency"] == "true":
        print("Uploading script to detect speed")
        cwd = os.getcwd()
        os.chdir(env["PROJECT_DIR"])
        executable = join(os.getenv("PLATFORMIO_CORE_DIR"), "penv", "bin", "platformio")
        subprocess.run([executable, "run", "--target", "upload", "--environment", "microdetect"], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        os.chdir(cwd)
        dev = None
        while not dev:
            dev = libusb_package.find(idVendor=0x1209, idProduct=0x2883)
            pass
        sleep(0.5)
        env.AutodetectUploadPort()
        port = env.subst("$UPLOAD_PORT")
        s = Serial(port=port, baudrate=115200)
        rate = f"{s.readline().decode('utf-8').strip()}000000"
        print(rate)
        # rate = usb.util.get_string(dev, dev.iProduct, 0x0409).split("\x00")[0].rpartition(" - ")[2]
        rate = f"{rate}L"
        env["BOARD_F_CPU"] = rate

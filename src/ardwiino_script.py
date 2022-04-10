#!/usr/bin/env python3
from pprint import pp
import subprocess
import sys
import re
try:
    import usb.core
    import usb.util
except ImportError:
    subprocess.check_call([sys.executable, "-m", "pip", "install", "pyusb"])

import usb.core
import usb.util
from platformio import fs
from platformio.util import get_serial_ports
from platformio.project.config import ProjectConfig
from platformio.commands.run.processor import EnvironmentProcessor
from platformio.commands.run.helpers import clean_build_dir
import os
REBOOT=48
BOOTLOADER=49
BOOTLOADER_SERIAL=50

def launch_dfu():
    dev = usb.core.find(idVendor=0x03eb)
    dev.ctrl_transfer(0xA1, 3, 0, 0, 8)
    command = [0x04, 0x03, 0x00]
    dev.ctrl_transfer(0x21, 1, 0, 0, command)

class Context:
    def __init__(self):
        self.meta = ""

Import("env")
def before_upload(source, target, env):
    b_request = None
    exists = False
    if "/arduino_uno_mega_usb" in str(source[0]):
        idVendor = 0x03eb
        idProduct = None
        b_request = BOOTLOADER
        if (usb.core.find(idVendor=0x1209, idProduct=0x2883)):
            env.AutodetectUploadPort()
            env.TouchSerialPort("$UPLOAD_PORT", 1200)
    if "/arduino_uno/" in str(source[0]):
        b_request = BOOTLOADER_SERIAL
        idVendor = 0x1209
        idProduct = 0x2883
        exists = usb.core.find(idProduct=idProduct, idVendor=idVendor)
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
    if b_request:
        args = {"idVendor": idVendor}
        if idProduct:
            args["idProduct"] = idProduct
        
        before_ports = get_serial_ports()
        while not usb.core.find(**args):
            pass
        if not exists and idProduct == 0x2883:
            env.Replace(UPLOAD_PORT=env.WaitForNewSerialPort(before_ports))

def post_upload(source, target, env):
    if "/arduino_uno/" in str(source[0]):
        env.TouchSerialPort("$UPLOAD_PORT", 2400)
    if "/arduino_uno_mega_usb" in str(source[0]):
        launch_dfu()
def before_build(source, target, env):
    print("BEFORE BUILD!")
    print(target)
env.AddPreAction("upload", before_upload)
env.AddPostAction("upload", post_upload)
if "upload" in BUILD_TARGETS:
    upload_options = env.BoardConfig().get("upload", {})
    if upload_options["detect_frequency"]:
        print("Uploading script to detect speed")
        project_dir = env["PROJECT_DIR"]
        with fs.cd(project_dir):
            config = ProjectConfig.get_instance(
                os.path.join(project_dir, "platformio.ini")
            )
        config.validate()
        processor = EnvironmentProcessor(Context(), "microdetect",config,["upload"],"",False,False, 1)
        processor.process()
        dev = None
        while not dev:
            dev = usb.core.find(idVendor=0x1209, idProduct=0x2883)
            pass
        rate = usb.util.get_string(dev, dev.iProduct).split("\x00")[0].rpartition(" - ")[2]
        rate = f"{rate}L"
        # actual_env = re.search(r".pio\/build\/(.+?)\/firmware.hex", str(source[0])).group(1)
        # processor = EnvironmentProcessor(Context(), actual_env,config,["upload"],"",True,True, 1)
        # processor.options["board_build.f_cpu"] = rate
        # processor.process()
        print(env["BOARD_F_CPU"])
        env["BOARD_F_CPU"] = rate
        # env.AutodetectUploadPort()
        # env.TouchSerialPort("$UPLOAD_PORT", 1200)
if env["BOARD_F_CPU"] == "skip":
    env["BOARD_F_CPU"] = "16000000L"


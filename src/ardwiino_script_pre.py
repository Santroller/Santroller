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

class Context:
    def __init__(self):
        self.meta = ""

Import("env")
if "upload" in BUILD_TARGETS:
    upload_options = env.BoardConfig().get("upload", {})
    print(upload_options)
    if "detect_frequency" in upload_options and upload_options["detect_frequency"] == "true":
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
        env["BOARD_F_CPU"] = rate

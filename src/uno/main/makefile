#
#             LUFA Library
#     Copyright (C) Dean Camera, 2012.
#
#  dean [at] fourwalledcubicle [dot] com
#           www.lufa-lib.org
#
# --------------------------------------
#         LUFA Project Makefile.
# --------------------------------------

MCU          = atmega328p
ARCH         = AVR8
BOARD        = UNO
F_CPU        = 16000000
F_USB        = $(F_CPU)
OPTIMIZATION = s
# Compile everything that isnt related to the output stage, as that comes under usb
SRC =  $(shell find ../../shared/input -type f -name '*.c*') $(shell find ../../shared/config -type f -name '*.c*') $(shell find . -type f -name '*.c*') 
LUFA_PATH    = ../../../lib/lufa/LUFA
CC_FLAGS     = -DUSE_LUFA_CONFIG_HEADER -I../../shared/output/usb -Werror -flto -fuse-linker-plugin
CPP_FLAGS	 = -std=c++11
LD_FLAGS     =
OBJDIR		 = obj
BIN		 	 = bin
TARGET       = $(BIN)/ardwiino-uno-main-${MCU}-${F_CPU}
AVRDUDE_PROGRAMMER = usbasp
AVRDUDE_FLAGS = -b 115200 -P /dev/ttyACM0
$(info $(shell mkdir -p $(BIN)))

# Default target
all:

# Include LUFA build script makefiles
include $(LUFA_PATH)/Build/lufa_core.mk
include $(LUFA_PATH)/Build/lufa_sources.mk
include $(LUFA_PATH)/Build/lufa_build.mk
include $(LUFA_PATH)/Build/lufa_cppcheck.mk
include $(LUFA_PATH)/Build/lufa_doxygen.mk
include $(LUFA_PATH)/Build/lufa_dfu.mk
include $(LUFA_PATH)/Build/lufa_hid.mk
include $(LUFA_PATH)/Build/lufa_avrdude.mk
include $(LUFA_PATH)/Build/lufa_atprogram.mk
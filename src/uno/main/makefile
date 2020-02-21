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
ARDWIINO_BOARD = uno
VERSION = $(shell git describe --abbrev=0 --tags)
SIGNATURE = ardwiino
F_CPU        = 16000000
F_USB        = $(F_CPU)
OPTIMIZATION = s
# Compile everything that isnt related to the output stage, as that comes under usb
SRC =  ../../shared/output/reports.c $(shell find ../../shared/input -type f -name '*.c*') $(shell find ../../shared/config -type f -name '*.c*') $(shell find . -type f -name '*.c*') $(shell find ../../../lib/FastLED -type f -name '*.c*')
LUFA_PATH    = ../../../lib/lufa/LUFA
CC_FLAGS     = -Werror -flto -fuse-linker-plugin -I. -DSIGNATURE='"${SIGNATURE}"' -DVERSION='"${VERSION}"' -DMCU='"${MCU}"' -DARDWIINO_BOARD='"${ARDWIINO_BOARD}"'  -include ../../shared/arduino.h
CPP_FLAGS	 = -std=c++11 -I. -include ../../shared/arduino.h
LD_FLAGS     =
OBJDIR		 = obj
BIN		 	 = bin
TARGET       = $(BIN)/ardwiino-uno-main-${MCU}-${F_CPU}
AVRDUDE_PROGRAMMER = arduino
AVRDUDE_FLAGS = -b 115200 -P /dev/ttyACM0
OUT_DIR 	 = ../../../output
$(info $(shell mkdir -p $(BIN)))
$(info $(shell echo e > /dev/ttyACM0))

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
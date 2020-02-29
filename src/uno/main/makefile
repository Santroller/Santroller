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
OPTIMIZATION = s
F_CPU        = 16000000
F_USB        = $(F_CPU)

ARDWIINO_BOARD = uno
VERSION = $(shell git describe --abbrev=0 --tags)
SIGNATURE = ardwiino

MOSI         = PB3
MOSI_DDR     = DDRB
MOSI_PORT    = PORTB
SS           = PB2
SS_DDR       = DDRB
SS_PORT      = PORTB
SCK          = PB5
SCK_DDR      = DDRB

# Compile everything that isnt related to the output stage, as that comes under usb
SRC =  ../../shared/output/reports.c ../../shared/defines.c ../../shared/output/serial_handler.c ../../shared/output/bootloader/bootloader.c $(shell find ../../shared/input -type f -name '*.c*') $(shell find ../../shared/config -type f -name '*.c*') $(shell find . -type f -name '*.c*') $(shell find ../../../lib/Adafruit_NeoPixel_Static -type f -name '*.c*')
LUFA_PATH    = ../../../lib/lufa/LUFA
CC_FLAGS     = -Werror -flto -fuse-linker-plugin -I. -I ../../shared -include ../../shared/Arduino.h -DARDUINO=1000  
CC_FLAGS    += -DSIGNATURE='"${SIGNATURE}"' -DVERSION='"${VERSION}"' -DMCU='"${MCU}"' -DARDWIINO_BOARD='"${ARDWIINO_BOARD}"' -DUSB_MCU='""' -DF_CPU_USB='""'
CC_FLAGS	+= -DMOSI=$(MOSI) -DMOSI_DDR=$(MOSI_DDR) -DMOSI_PORT=$(MOSI_PORT)
CC_FLAGS    += -DSS=$(SS) -DSS_DDR=$(SS_DDR) -DSS_PORT=$(SS_PORT)
CC_FLAGS    += -DSCK=$(SCK) -DSCK_DDR=$(SCK_DDR)
LD_FLAGS     =
OBJDIR		 = obj
BIN		 	 = bin
TARGET       = $(BIN)/ardwiino-uno-main-${MCU}-${F_CPU}

AVRDUDE_PROGRAMMER = arduino
AVRDUDE_FLAGS = -b 115200 -P /dev/ttyACM0

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
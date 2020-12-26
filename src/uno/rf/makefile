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
VARIANT		 = uno

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
SRC =   ../../../lib/avr-nrf24l01/src/nrf24l01.c ../../shared/bootloader/bootloader.c ../../shared/controller/guitar_includes.c ../../shared/output/reports.c ../../shared/util/util.c ../../shared/output/serial_handler.c $(shell find ../../shared/output/reports/ -type f -name '*.c*') $(shell find ../../../lib/spi -type f -name '*.c*') $(shell find ../../../lib/i2c -type f -name '*.c*') $(shell find ../../../lib/mpu6050 -type f -name '*.c*') ../../shared/leds/leds.c $(shell find ../../shared/input -type f -name '*.c*') $(shell find ../../shared/config -type f -name '*.c*') $(shell find . -type f -name '*.c*')
LUFA_PATH    = ../../../lib/lufa/LUFA
CC_FLAGS     = -Werror -flto -fuse-linker-plugin -I. -I ../shared -I ../../shared -I../../../lib -I../../variants/${VARIANT}
CC_FLAGS    += -DSIGNATURE='"${SIGNATURE}"' -DVERSION='"${VERSION}"' ${MULTI_ADAPTOR} -DMCU='"${MCU}"' -DARDWIINO_BOARD='"${ARDWIINO_BOARD}"'
LD_FLAGS     = -Wl,--section-start=.data=$(RAM_OFFSET) $(REGS) -flto -fuse-linker-plugin -Wl,--section-start=.rfrecv=0x7B00
OBJDIR		 = obj
BIN		 	 = bin
TARGET       = $(BIN)/ardwiino-${ARDWIINO_BOARD}-main-${MCU}-${F_CPU}-rf
RAM_OFFSET            = 0x800282
AVRDUDE_PROGRAMMER = arduino
AVRDUDE_FLAGS = -b 115200 -P /dev/ttyACM1

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
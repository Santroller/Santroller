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

MCU          = atmega32u4
ARCH         = AVR8
BOARD        = AVR_MICRO
OPTIMIZATION = s
F_CPU        = 8000000
F_USB        = $(F_CPU)

ARDWIINO_BOARD = micro
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
PID=$(if $(filter-out $(F_CPU),8000000),0x9206,0x9204)
# Compile everything that isnt related to the output stage, as that comes under usb
SRC =   ../../lib/avr-nrf24l01/src/nrf24l01.c ../shared/bootloader/bootloader.c ../shared/controller/guitar_includes.c ../shared/output/reports.c ../shared/util/util.c ../shared/output/serial_handler.c $(shell find ../shared/output/reports/ -type f -name '*.c*') $(shell find ../../lib/spi -type f -name '*.c*') $(shell find ../../lib/i2c -type f -name '*.c*') $(shell find ../../lib/mpu6050 -type f -name '*.c*') ../shared/leds/leds.c $(shell find ../shared/input -type f -name '*.c*') $(shell find ../shared/config -type f -name '*.c*') $(shell find . -type f -name '*.c*')  $(LUFA_SRC_USBCLASS) $(LUFA_SRC_USB)
LUFA_PATH    = ../../lib/lufa/LUFA
CC_FLAGS     = -DUSE_LUFA_CONFIG_HEADER -I../shared/output -I ../shared  -Werror -flto -fuse-linker-plugin -I. -I ../shared -I ../shared -I../../lib
CC_FLAGS    += -DSIGNATURE='"${SIGNATURE}"' -DVERSION='"${VERSION}"' ${MULTI_ADAPTOR} -DMCU='"${MCU}"' -DARDWIINO_BOARD='"${ARDWIINO_BOARD}"' -DPID=${PID}
LD_FLAGS     = -Wl,--section-start=.data=$(RAM_OFFSET) $(REGS) -flto -fuse-linker-plugin
OBJDIR		 = obj
BIN		 	 = bin
TARGET       = $(BIN)/ardwiino-${ARDWIINO_BOARD}-main-${MCU}-${F_CPU}$(if ${MULTI_ADAPTOR},-multi,)
RAM_OFFSET            = 0x800282
AVRDUDE_PROGRAMMER = avr109
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
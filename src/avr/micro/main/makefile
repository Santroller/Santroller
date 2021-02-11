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
F_CPU 		 = 16000000
F_USB		 = ${F_CPU}

ARDWIINO_BOARD = micro
VARIANT = micro
VERSION = $(shell git describe --abbrev=0 --tags)
VERSION_LIST = $(subst ., ,$(subst version-,,$(VERSION)))
VERSION_MAJOR = $(word 1,$(VERSION_LIST))
VERSION_MINOR = $(word 2,$(VERSION_LIST))
VERSION_REVISION = $(word 3,$(VERSION_LIST))
SIGNATURE = ardwiino
MOSI         = PB2
MOSI_DDR     = DDRB
MOSI_PORT    = PORTB
SS           = PB0
SS_DDR       = DDRB
SS_PORT      = PORTB
SCK          = PB1
SCK_DDR      = DDRB

SRC = ../../../../lib/avr-nrf24l01/src/nrf24l01.c $(shell find ../../../shared -type f -name '*.c*') main.c $(shell find ../../../../lib/spi -type f -name '*.c*') $(shell find ../../../../lib/i2c -type f -name '*.c*') $(shell find ../../../../lib/mpu6050 -type f -name '*.c*') $(LUFA_SRC_USBCLASS) $(LUFA_SRC_USB)
LUFA_PATH    = ../../../../lib/lufa/LUFA
CC_FLAGS     = -DUSE_LUFA_CONFIG_HEADER -I../../../shared/output -I../../variants/${VARIANT} -I ../../../shared -I../../../../lib -Werror $(REGS) -DARDUINO=1000  -flto -fuse-linker-plugin -ffast-math
CC_FLAGS     += -DARDWIINO_BOARD='"${ARDWIINO_BOARD}"' 
CC_FLAGS 	 += -DSIGNATURE='"${SIGNATURE}"' -DVERSION='"${VERSION}"' ${MULTI_ADAPTOR} -DVERSION_MAJOR='${VERSION_MAJOR}' -DVERSION_MINOR='${VERSION_MINOR}' -DVERSION_REVISION='${VERSION_REVISION}' -DMCU='"${MCU}"'
LD_FLAGS     = $(REGS) -DARDUINO=1000 -DMCU='"${MCU}"' -flto -fuse-linker-plugin 
OBJDIR		 = obj
BIN		 	 = bin
TARGET       = $(BIN)/ardwiino-${ARDWIINO_BOARD}-${MCU}-${F_CPU}$(if ${MULTI_ADAPTOR},-multi,)

AVRDUDE_PROGRAMMER = avr109
AVRDUDE_FLAGS = -P /dev/ttyACM0 reset
# ----- No changes should be necessary below this line -----
$(info $(SRC))
$(info $(shell mkdir -p $(BIN)))

# Default target
all:

# Reserved registers for faster USB-Serial convertion
# The lower, the better (except 0 and 1)
# r2-5 used for faster USART ISRs
# r6 used for 32u4 optimization
REGS  = -ffixed-r2 -ffixed-r3 -ffixed-r4 -ffixed-r5 -ffixed-r6
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
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

MCU          = atmega16u2
ARCH         = AVR8
BOARD        = UNO
OPTIMIZATION = s
F_CPU        = 16000000
F_USB        = $(F_CPU)

ARDWIINO_BOARD = uno
VERSION = $(shell git describe --abbrev=0 --tags)
VERSION_LIST = $(subst ., ,$(subst version-,,$(VERSION)))
VERSION_MAJOR = $(word 1,$(VERSION_LIST))
VERSION_MINOR = $(word 2,$(VERSION_LIST))
VERSION_REVISION = $(word 3,$(VERSION_LIST))
SIGNATURE = ardwiino

SRC          = main.c ../../shared/controller/guitar_includes.c ../../shared/output/descriptors.c ../../shared/output/control_requests.c ../../shared/bootloader/bootloader.c $(LUFA_SRC_USBCLASS) $(LUFA_SRC_USB)
LUFA_PATH    = ../../../lib/lufa/LUFA
CPP_FLAGS	 = -std=c++11
CC_FLAGS     = -DUSE_LUFA_CONFIG_HEADER -I ../shared -I ../../../lib -I ../../shared -I../../shared/output -Werror -DMCU='"${MCU}"' $(REGS) -flto -fuse-linker-plugin
CC_FLAGS    += -DSIGNATURE='"${SIGNATURE}"' -DVERSION='"${VERSION}"' ${MULTI_ADAPTOR} -DVERSION_MAJOR='${VERSION_MAJOR}' -DVERSION_MINOR='${VERSION_MINOR}' -DVERSION_REVISION='${VERSION_REVISION}' -DMCU='"${MCU}"' -DARDWIINO_BOARD='"${ARDWIINO_BOARD}"' -DUSB_MCU='""' -DF_CPU_USB='""'
LD_FLAGS     = -Wl,--section-start=.data=$(RAM_OFFSET) $(REGS) -flto -fuse-linker-plugin
OBJDIR		 = obj
BIN		 	 = bin
TARGET       = $(BIN)/ardwiino-${ARDWIINO_BOARD}-usb-${MCU}-${F_CPU}$(if ${MULTI_ADAPTOR},-multi,)

AVRDUDE_PROGRAMMER = arduino
AVRDUDE_FLAGS = -b 57600 -P /dev/ttyACM0 reset
RAM_OFFSET            = 0x800282
# Reserved registers for faster USB-Serial convertion
# The lower, the better (except 0 and 1)
# r2-5 used for faster USART ISRs
# r6 used for 32u4 optimization
REGS  = -ffixed-r2 -ffixed-r3 -ffixed-r4 -ffixed-r5 -ffixed-r6

$(info $(shell mkdir -p $(BIN)))
# Default target
all:
dfu:
	echo j > /dev/ttyACM0 || true
	sleep 1
	dfu-programmer $(MCU) erase || true
	make || true
	dfu-programmer $(MCU) flash bin/ardwiino-${ARDWIINO_BOARD}-usb-$(MCU)-16000000.hex || true
	sleep 2
	dfu-programmer $(MCU) launch || true
	

# Include LUFA build script makefiles
include $(LUFA_PATH)/Build/lufa_core.mk
include $(LUFA_PATH)/Build/lufa_sources.mk
include $(LUFA_PATH)/Build/lufa_build.mk
include $(LUFA_PATH)/Build/lufa_cppcheck.mk
include $(LUFA_PATH)/Build/lufa_doxygen.mk
include $(LUFA_PATH)/Build/lufa_hid.mk
include $(LUFA_PATH)/Build/lufa_avrdude.mk
include $(LUFA_PATH)/Build/lufa_atprogram.mk

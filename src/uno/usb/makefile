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
BOARD        = USER
ARDWIINO_BOARD = uno-usb
F_CPU        = 16000000
F_USB        = $(F_CPU)
OPTIMIZATION = s
SRC          = main.c ../../shared/output/usb/Descriptors.c ../../shared/output/control_requests.c $(LUFA_SRC_USBCLASS) $(LUFA_SRC_USB)
LUFA_PATH    = ../../../lib/lufa/LUFA
CPP_FLAGS	 = -std=c++11
CC_FLAGS     = -DUSE_LUFA_CONFIG_HEADER -I../../shared/output/usb -Werror -DARDWIINO_BOARD='"${ARDWIINO_BOARD}"'  -DMCU='"${MCU}"' $(REGS) -flto -fuse-linker-plugin
LD_FLAGS     = -Wl,--section-start=.data=$(RAM_OFFSET) $(REGS) -flto -fuse-linker-plugin
OBJDIR		 = obj
BIN		 	 = bin
TARGET       = $(BIN)/ardwiino-uno-usb-${MCU}-${F_CPU}
AVRDUDE_PROGRAMMER = arduino
AVRDUDE_FLAGS = -b 57600 -P /dev/ttyACM0 reset

# Add an offset of 256 + 128 bytes to RAMSTART for the ringbuffers
# +1 byte offset for old MagicBootKey support (0x280)
# +1 for better data aligning or in case a 2byte key is written
# u2 and u4 Series (RAMSTART = 0x100):
RAM_OFFSET            = 0x800290

# Reserved registers for faster USB-Serial convertion
# The lower, the better (except 0 and 1)
# r2-5 used for faster USART ISRs
# r6 used for 32u4 optimization
REGS  = -ffixed-r2 -ffixed-r3 -ffixed-r4 -ffixed-r5 -ffixed-r6
$(info $(shell mkdir -p $(BIN)))

# Default target
all:

dfu16:
	echo j > /dev/ttyACM0 || true
	sleep 1
	dfu-programmer $(MCU) erase || true
	sleep 1
	make dfu || true
	sleep 2
	dfu-programmer $(MCU) launch --no-reset || true

dfu8:
	echo j > /dev/ttyACM0 || true
	sleep 1
	dfu-programmer at90usb82 erase || true
	sleep 1
	dfu-programmer at90usb82 flash bin/ardwiino-uno-usb-atmega8u2-16000000.hex || true
	sleep 2
	dfu-programmer at90usb82 launch --no-reset || true


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

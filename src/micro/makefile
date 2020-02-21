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
ARDWIINO_BOARD = micro
F_CPU 		 = 8000000
F_USB		 = ${F_CPU}
OPTIMIZATION = s
SRC = $(shell find ../shared -type f -name '*.c*') main.c  $(shell find ../../lib/FastLED -type f -name '*.c*') $(LUFA_SRC_USBCLASS) $(LUFA_SRC_USB)
LUFA_PATH    = ../../lib/lufa/LUFA
CC_FLAGS     = -DUSE_LUFA_CONFIG_HEADER -I../shared/output/usb -Werror -DARDWIINO_BOARD='"${ARDWIINO_BOARD}"' $(REGS) -flto -fuse-linker-plugin  -include ../shared/arduino.h
LD_FLAGS     = -Wl,--section-start=.data=$(RAM_OFFSET) $(REGS) -DMCU='"${MCU}"' -flto -fuse-linker-plugin  -include ../shared/arduino.h
OBJDIR		 = obj
BIN		 	 = bin
TARGET       = $(BIN)/ardwiino-${ARDWIINO_BOARD}-${MCU}-${F_CPU}
AVRDUDE_PROGRAMMER = avr109
AVRDUDE_FLAGS = -P /dev/ttyACM0 reset
# ----- No changes should be necessary below this line -----
$(info $(SRC))
$(info $(shell mkdir -p $(BIN)))

# Default target
all:
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
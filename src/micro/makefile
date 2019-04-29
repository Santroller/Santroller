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
F_CPU        = 8000000
F_USB        = $(F_CPU)
OPTIMIZATION = s
SRC = $(shell find ../shared -type f -name '*.c' -not -path "../shared/io/lufa/lufa/*") $(shell find ../shared -type f -name '*.cpp' -not -path "../shared/io/lufa/lufa/*") $(shell find . -type f -name '*.c') $(shell find . -type f -name '*.cpp') $(LUFA_SRC_USBCLASS) $(LUFA_SRC_USB)
LUFA_PATH    = ../shared/io/lufa/lufa/LUFA
CC_FLAGS     = -DUSE_LUFA_CONFIG_HEADER -I../shared/io/lufa/ 
CPP_FLAGS	 = -std=c++11
LD_FLAGS     =
OBJDIR		 = obj
BIN		 	 = bin
TARGET       = $(BIN)/Ardwiino
AVRDUDE_PROGRAMMER = avr109
AVRDUDE_FLAGS = -P /dev/ttyACM0
# ----- No changes should be necessary below this line -----
$(info $(SRC))
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
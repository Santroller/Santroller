MCU          = atmega16u2
BOARD        = UNO
F_CPU        = 16000000
VARIANT		 = uno
PORT 		 = /dev/ttyUSB0

ARDWIINO_BOARD = uno

SRC = main.c

AVRDUDE_PROGRAMMER = arduino
AVRDUDE_FLAGS = -b 57600 -P ${PORT}
CC_FLAGS = -I../shared
RAM_OFFSET   = 0x800282
LD_FLAGS     = -Wl,--section-start=.data=$(RAM_OFFSET)
MCU_TYPE = usb

PROJECT_ROOT = ../../../../

# Default target
all:

include ${PROJECT_ROOT}/src/avr/usb-src.mk
include ${PROJECT_ROOT}/src/avr/main.mk
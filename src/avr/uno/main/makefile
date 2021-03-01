MCU          = atmega328p
BOARD        = UNO
F_CPU        = 16000000
VARIANT		 = uno
PORT 		 = /dev/ttyUSB0

ARDWIINO_BOARD = uno
SIGNATURE = ardwiino

SRC = main.c

AVRDUDE_PROGRAMMER = arduino
AVRDUDE_FLAGS = -b 115200 -P ${PORT}
CC_FLAGS = -I../shared
MCU_TYPE = main

PROJECT_ROOT = ../../../../

RAM_OFFSET   = 0x800282
LD_FLAGS     = -Wl,--section-start=.data=$(RAM_OFFSET)
# Default target
all:

include ${PROJECT_ROOT}/src/avr/main-src.mk
include ${PROJECT_ROOT}/src/avr/main.mk
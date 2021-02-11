MCU          = atmega328p
BOARD        = UNO
F_CPU        = 16000000
VARIANT		 = uno
PORT 		 = /dev/ttyUSB0

ARDWIINO_BOARD = mini
SIGNATURE = ardwiino

SRC = main.c

AVRDUDE_PROGRAMMER = arduino
AVRDUDE_FLAGS = -b 57600 -P ${PORT}
CC_FLAGS = -I../shared
MCU_TYPE = main

PROJECT_ROOT = ../../../../
$(info $(SRC))
$(info $(shell mkdir -p $(BIN)))

# Default target
all:

include ${PROJECT_ROOT}/src/avr/main-src.mk
include ${PROJECT_ROOT}/src/avr/main.mk
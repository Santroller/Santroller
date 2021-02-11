MCU          = atmega328p
BOARD        = UNO
F_CPU        = 8000000
VARIANT		 = uno
PORT 		 = /dev/ttyUSB0

ARDWIINO_BOARD = mini
SIGNATURE = ardwiino

SRC = main.c

CC_FLAGS=-DPID=${PID} -DRF_TX=true
LD_FLAGS= -Wl,--section-start=.rfrecv=0x7000
AVRDUDE_PROGRAMMER = arduino
AVRDUDE_FLAGS = -b 57600 -P ${PORT}
RF = true

PROJECT_ROOT = ../../../../
$(info $(SRC))
$(info $(shell mkdir -p $(BIN)))

# Default target
all:

include ${PROJECT_ROOT}/src/avr/main-src.mk
include ${PROJECT_ROOT}/src/avr/main.mk
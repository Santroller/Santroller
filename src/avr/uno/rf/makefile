MCU          = atmega328p
BOARD        = UNO
F_CPU        = 16000000
VARIANT		 = uno
PORT 		 = /dev/ttyUSB0

ARDWIINO_BOARD = uno
SIGNATURE = ardwiino

SRC = main.c
# FLASH_END of the mini is 0x7000, while the other variants have it at 0x7B00
# TODO: this is probably wrong for the mega?
RF_LOC = $(if $(findstring mini,$(VARIANT)),0x7000,0x7B00)
CC_FLAGS=-DPID=${PID} -DRF_TX=true
LD_FLAGS= -Wl,--section-start=.rfrecv=${RF_LOC}
AVRDUDE_PROGRAMMER = arduino
AVRDUDE_FLAGS = -b 57600 -P ${PORT}
RF = true

PROJECT_ROOT = ../../../../

# Default target
all:

include ${PROJECT_ROOT}/src/avr/main-src.mk
include ${PROJECT_ROOT}/src/avr/main.mk
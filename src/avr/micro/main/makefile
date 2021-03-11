MCU          = atmega32u4
BOARD        = AVR_MICRO
F_CPU 		 = 8000000

ARDWIINO_BOARD = micro
VARIANT = micro

AVRDUDE_PROGRAMMER = avr109
AVRDUDE_FLAGS = -P /dev/ttyACM0 reset

PROJECT_ROOT = ../../../../

SRC = main.c

# Default target
all:

include ${PROJECT_ROOT}/src/avr/usb-src.mk
include ${PROJECT_ROOT}/src/avr/main-src.mk
include ${PROJECT_ROOT}/src/avr/main.mk
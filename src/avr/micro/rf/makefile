
MCU          = atmega32u4
BOARD        = AVR_MICRO
F_CPU 		 = 16000000
RF			 = true

ARDWIINO_BOARD = micro
VARIANT = micro

AVRDUDE_PROGRAMMER = avr109
AVRDUDE_FLAGS = -P /dev/ttyACM0 reset

PROJECT_ROOT = ../../../../

SRC = main.c Descriptors.c $(LUFA_SRC_USBCLASS) $(LUFA_SRC_USB)

PID=$(if $(filter-out $(F_CPU),8000000),0x9206,0x9204)

CC_FLAGS=-DPID=${PID} -DRF_TX=true
LD_FLAGS= -Wl,--section-start=.rfrecv=0x7B00

$(info $(SRC))
$(info $(shell mkdir -p $(BIN)))

# Default target
all:

include ${PROJECT_ROOT}/src/avr/main-src.mk
include ${PROJECT_ROOT}/src/avr/main.mk
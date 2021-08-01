SRC += $(LUFA_SRC_USBCLASS) $(LUFA_SRC_USB)
SRC += $(PROJECT_ROOT)/src/lib/descriptors.c 
SRC += $(PROJECT_ROOT)/src/lib/report_descriptors.c
SRC += $(PROJECT_ROOT)/src/lib/lib_main.c
SRC += $(PROJECT_ROOT)/src/lib/usb.c
SRC += ${PROJECT_ROOT}/src/avr/lib/pin.c
SRC += ${PROJECT_ROOT}/src/avr/lib/timer.c
AVRDUDE_PROGRAMMER = avr109
AVRDUDE_FLAGS = -P /dev/ttyACM0 reset
BOARD        = AVR_MICRO
CC_FLAGS	 += -DNUM_DIGITAL_PINS=24 -DNUM_ANALOG_INPUTS=12 -DPORTS=5
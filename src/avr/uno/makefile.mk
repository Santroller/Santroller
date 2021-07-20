SRC += $(PROJECT_ROOT)/src/lib/descriptors.c
SRC += $(PROJECT_ROOT)/src/lib/report_descriptors.c
SRC += $(PROJECT_ROOT)/src/lib/lib_main.c
SRC += $(PROJECT_ROOT)/src/lib/usb.c
SRC += $(LUFA_SRC_SERIAL)
AVRDUDE_PROGRAMMER = arduino
AVRDUDE_FLAGS = -P /dev/ttyACM0 reset
# To allow using the LUFA Serial library on the 328p, we have to map all of the serial registers from serial 1 to serial 0, as the 328p only has the one serial interface
CC_FLAGS 	 += -DUBRR1=UBRR0 -DUCSR1A=UCSR0A -DUCSR1B=UCSR0B -DUCSR1C=UCSR0C -DUCSZ10=UCSZ00 -DUCSZ11=UCSZ01 -DU2X1=U2X0 -DTXEN1=TXEN0 -DRXEN1=RXEN0 -DRXC1=RXC0 -DRXCIE1=RXCIE0 -DTXC1=TXC0 -DUDR1=UDR0 -DUDRE1=UDRE0
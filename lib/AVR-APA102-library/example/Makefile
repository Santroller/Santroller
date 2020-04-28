# The number of LED you want to light up
LED_COUNT = 144

# Uncomment this to shift the first lit LED further down the strip.
# Your strip should have at least (STARTING_LED + LED_COUNT - 1) LEDs.
#STARTING_LED = 137

# Uncomment this to limit the amount of current used by the strip by scaling
# the LED duty cycles. In miliamperes.
CURRENT_LIMIT = 1000

# If you're using an external oscillator, change this to your clock frequency.
F_CPU = 8000000UL

# ATmega168a PDIP-28
MCU = atmega168
MOSI = PB3
MOSI_DDR = DDRB
MOSI_PORT = PORTB
SS = PB2
SS_DDR = DDRB
SS_PORT = PORTB
SCK = PB5
SCK_DDR = DDRB


PROGRAMMER = usbtiny

LIBDIR = ../src


CC = avr-gcc
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump
AVRSIZE = avr-size
AVRDUDE = avrdude

TARGET = main

SOURCES=$(wildcard *.c $(LIBDIR)/*.c)
OBJECTS=$(SOURCES:.c=.o)
HEADERS=$(SOURCES:.c=.h)

CPPFLAGS  = -I. -I$(LIBDIR)
CPPFLAGS += -DF_CPU=$(F_CPU)
CPPFLAGS += -DMOSI=$(MOSI) -DMOSI_DDR=$(MOSI_DDR) -DMOSI_PORT=$(MOSI_PORT)
CPPFLAGS += -DSS=$(SS) -DSS_DDR=$(SS_DDR) -DSS_PORT=$(SS_PORT)
CPPFLAGS += -DSCK=$(SCK) -DSCK_DDR=$(SCK_DDR)
CPPFLAGS += -DLED_COUNT=$(LED_COUNT)
ifdef STARTING_LED
CPPFLAGS += -DSTARTING_LED=$(STARTING_LED)
endif

ifdef CURRENT_LIMIT
CURRENT_PER_LED = $$(( $(CURRENT_LIMIT) / $(LED_COUNT) ))
ifeq ($(shell test $(CURRENT_PER_LED) -lt 60; echo $$?), 0)
CPPFLAGS += -DCURRENT_PER_LED=$(CURRENT_PER_LED)UL
endif
endif

CFLAGS  = -Os -g -std=gnu99 -Wall
CFLAGS += -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums
CFLAGS += -ffunction-sections -fdata-sections

LDFLAGS  = -Wl,-Map,$(TARGET).map
LDFLAGS += -Wl,--gc-sections

TARGET_ARCH = -mmcu=$(MCU)


%.o: %.c $(HEADERS) Makefile
	 $(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c -o $@ $<;

$(TARGET).elf: $(OBJECTS)
	$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LDLIBS) -o $@

%.hex: %.elf
	 $(OBJCOPY) -j .text -j .data -O ihex $< $@


.PHONY: all flash clean

all: $(TARGET).hex

flash: $(TARGET).hex
	$(AVRDUDE) -c $(PROGRAMMER) -p $(MCU) -U flash:w:$<

clean:
	rm -f *.hex *.elf *.map
	rm -f *.o $(LIBDIR)/*.o

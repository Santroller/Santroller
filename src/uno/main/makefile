# Hardware
MCU     = atmega328p # see `make show-mcu`
OSC     = 16000000UL
PROJECT = bin/main

# ----- These configurations are quite likely not to be changed -----

# Binaries
GCC     = avr-gcc
G++     = avr-g++
RM      = rm -f
AVRDUDE = avrdude

# Files
EXT_C   = c
EXT_C++ = cpp
EXT_ASM = asm

PGMER       = -c arduino -b 115200 -P /dev/ttyACM0
PGMERISP    = -c avrispv2 -P /dev/ttyACM0
DUDE = /usr/bin/avrdude -V -p $(MCU)
# ----- No changes should be necessary below this line -----
C_FILES = $(shell find ../../shared -type f -name '*.c' -not -path "../../shared/lufa/lufa/*") $(shell find ../../shared -type f -name '*.cpp' -not -path "../../shared/lufa/lufa/*") $(shell find . -type f -name '*.c') $(shell find . -type f -name '*.cpp')
OBJECTS = \
	$(patsubst %.$(EXT_C),obj/%.o,$(shell find ../../shared -type f -name '*.${EXT_C}' -not -path "../../shared/lufa/*" -not -path "../../shared/controller/output*")) \
	$(patsubst %.$(EXT_C++),obj/%.o,$(shell find ../../shared -type f -name '*.${EXT_C++}' -not -path "../../shared/lufa/*" -not -path "../../shared/controller/output*")) \
	obj/uart.o \
	obj/main.o


# TODO explain these flags, make them configurable
CFLAGS = $(INC)
CFLAGS += -Os
CFLAGS += -funsigned-char -funsigned-bitfields -fshort-enums
CFLAGS += -Wall -Wstrict-prototypes
CFLAGS += -DF_OSC=$(OSC)
CFLAGS += -DF_CPU=$(OSC)
CFLAGS += -DBOARD=UNO
CFLAGS += -mmcu=$(MCU)

C++FLAGS = $(INC)
C++FLAGS += -Os
C++FLAGS += -funsigned-char -funsigned-bitfields -fshort-enums
C++FLAGS += -Wall
C++FLAGS += -DF_OSC=$(OSC)
C++FLAGS += -DF_CPU=$(OSC)
C++FLAGS += -DBOARD=UNO
C++FLAGS += -mmcu=$(MCU)
C++FLAGS += -std=c++11

default: $(PROJECT).elf
	echo $(OBJECTS)

%.elf: $(OBJECTS)
	@mkdir -p $(@D)
	$(GCC) $(CFLAGS) $(OBJECTS) --output $@ $(LDFLAGS)

obj/%.o : %.$(EXT_C)
	@mkdir -p $(@D)
	$(GCC) $< $(CFLAGS) -c -o $@

obj/%.o : %.$(EXT_C++)
	@mkdir -p $(@D)
	$(G++) $< $(C++FLAGS) -c -o $@

clean:
	$(RM) -R bin obj

$(PROJECT).hex: $(PROJECT).elf
	avr-objcopy -j .text -j .data -O ihex $(PROJECT).elf $(PROJECT).hex
	avr-objcopy -j .eeprom --set-section-flags=.eeprom="alloc,load" -O ihex bin/main.elf bin/eeprom.hex

upload: $(PROJECT).hex
	$(DUDE) $(PGMER) -U flash:w:bin/main.hex
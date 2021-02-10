UNOMCU          = atmega16u2
UNOMCU8          = at90usb82
UNO_PID			=0x0001
MEGA2560_PID	=0x0010
MEGAADK_PID		=0x003f
all:

micro:
	$(MAKE) -C src/micro/main
	-stty -F /dev/ttyACM0 1200 || scripts/bootloader.py
	sleep 2
	$(MAKE) -C src/micro/main avrdude

micro_rf:
	$(MAKE) -C src/micro/rf
	-stty -F /dev/ttyACM0 1200 || scripts/bootloader.py
	sleep 2
	$(MAKE) -C src/micro/rf avrdude

gdb:
	-avarice -j /dev/ttyUSB0 -P atmega32 :4242 -r -R & avr-gdb ./src/micro/main/bin/ardwiino-micro-atmega32u4-8000000.elf

uno-8:
	$(MAKE) -C src/uno
	sleep 0.5
	scripts/bootloader.py || true
	sleep 1
	dfu-programmer $(UNOMCU8) erase || true
	dfu-programmer $(UNOMCU8) flash output/ardwiino-uno-usb-$(UNOMCU8)-16000000-usbserial.hex
	dfu-programmer $(UNOMCU8) launch
	sleep 1
	$(MAKE) -C src/uno/main avrdude
	sleep 1
	stty -F /dev/ttyACM0 1200
	sleep 1
	$(MAKE) -C src/uno/usb dfu MCU=$(UNOMCU8)

uno:
	$(MAKE) -C src/uno
	sleep 0.5
	scripts/bootloader.py || true
	sleep 1
	dfu-programmer $(UNOMCU) erase || true
	dfu-programmer $(UNOMCU) flash output/ardwiino-uno-usb-$(UNOMCU)-16000000-usbserial.hex
	dfu-programmer $(UNOMCU) launch
	sleep 1
	$(MAKE) -C src/uno/main avrdude
	sleep 1
	stty -F /dev/ttyACM0 1200
	sleep 1
	$(MAKE) -C src/uno/usb dfu MCU=$(UNOMCU)

uno-rftx:
	$(MAKE) -C src/uno/rf
	$(MAKE) -C src/uno/rf avrdude
	$(MAKE) -C src/uno/rf_rx
	$(MAKE) -C src/uno/rf_rx avrdude

mini-rf:
	$(MAKE) -C src/micro/main
	-stty -F /dev/ttyACM0 1200 || scripts/bootloader.py
	sleep 1
	$(MAKE) -C src/micro/main avrdude
	$(MAKE) -C src/mini/rf
	$(MAKE) -C src/mini/rf avrdude PORT=/dev/ttyUSB0
unomini-rf:
	$(MAKE) -C src/uno
	sleep 0.5
	scripts/bootloader.py || true
	sleep 1
	dfu-programmer $(UNOMCU) erase || true
	dfu-programmer $(UNOMCU) flash output/ardwiino-uno-usb-$(UNOMCU)-16000000-usbserial.hex
	dfu-programmer $(UNOMCU) launch
	sleep 1
	$(MAKE) -C src/uno/main avrdude
	sleep 1
	stty -F /dev/ttyACM0 1200
	sleep 1
	$(MAKE) -C src/uno/usb dfu MCU=$(UNOMCU)
	# sleep 1
	# stty -F /dev/ttyACM1 1200
	# sleep 1
	# $(MAKE) -C src/uno/usb dfu MCU=$(UNOMCU)
	$(MAKE) -C src/mini/rf
	$(MAKE) -C src/mini/rf avrdude PORT=/dev/ttyUSB0
uno-rf:
	$(MAKE) -C src/uno
	sleep 0.5
	-scripts/bootloader.py
	-stty -F /dev/ttyACM1 1200
	sleep 1
	dfu-programmer $(UNOMCU) erase || true
	dfu-programmer $(UNOMCU) flash output/ardwiino-uno-usb-$(UNOMCU)-16000000-usbserial.hex
	dfu-programmer $(UNOMCU) launch
	sleep 2
	$(MAKE) -C src/uno/main avrdude PORT=/dev/ttyACM1
	sleep 1
	stty -F /dev/ttyACM1 1200
	sleep 1
	$(MAKE) -C src/uno/usb dfu MCU=$(UNOMCU)
	$(MAKE) -C src/uno/rf
	$(MAKE) -C src/uno/rf avrdude PORT=/dev/ttyACM0
	
uno-rf2:
	$(MAKE) -C src/uno/rfRecTest
	$(MAKE) -C src/uno/rfRecTest avrdude
	$(MAKE) -C src/uno/rf
	$(MAKE) -C src/uno/rf avrdude

uno-rf3:
	$(MAKE) -C src/uno
	$(MAKE) -C src/uno/main avrdude PORT=/dev/ttyACM1
	# sleep 1
	# stty -F /dev/ttyACM1 1200
	# sleep 1
	# $(MAKE) -C src/uno/usb dfu MCU=$(UNOMCU)
	$(MAKE) -C src/uno/rf
	$(MAKE) -C src/uno/rf avrdude PORT=/dev/ttyACM0

clean:
	$(MAKE) -C src/micro/main clean
	$(MAKE) -C src/uno clean

build:
	$(MAKE) -C src/micro/main
	$(MAKE) -C src/uno

build-all:
# Arduino uno
# usb processor
	$(MAKE) -C src/uno/usb OBJDIR=obj/16u2 MCU=atmega16u2 ARDWIINO_BOARD=uno
	$(MAKE) -C src/uno/usb OBJDIR=obj/8u2 MCU=at90usb82 ARDWIINO_BOARD=uno
# usb processor - multi adaptor
	$(MAKE) -C src/uno/usb OBJDIR=obj/multi/16u2 MCU=atmega16u2 ARDWIINO_BOARD=uno MULTI_ADAPTOR=-DMULTI_ADAPTOR
	$(MAKE) -C src/uno/usb OBJDIR=obj/multi/8u2 MCU=at90usb82 ARDWIINO_BOARD=uno MULTI_ADAPTOR=-DMULTI_ADAPTOR
# usb processor - usbserial firmware
	$(MAKE) -C submodules/arduino-usbserial-bootloader OBJDIR=obj/16u2 MCU=atmega16u2 ARDUINO_MODEL_PID=${UNO_PID} A_BOARD=uno
	$(MAKE) -C submodules/arduino-usbserial-bootloader OBJDIR=obj/8u2 MCU=at90usb82 ARDUINO_MODEL_PID=${UNO_PID} A_BOARD=uno
# main processor
	$(MAKE) -C src/uno/main OBJDIR=obj/uno MCU=atmega328p ARDWIINO_BOARD=uno VARIANT=uno
# main processor - rf transmitter
	$(MAKE) -C src/uno/rf OBJDIR=obj/uno MCU=atmega328p ARDWIINO_BOARD=uno VARIANT=uno

# Arduino pro mini
# main processor - rf transmitter
	$(MAKE) -C src/mini/rf OBJDIR=obj/mini MCU=atmega328p ARDWIINO_BOARD=mini VARIANT=uno F_CPU=8000000
	$(MAKE) -C src/mini/rf OBJDIR=obj/mini MCU=atmega328p ARDWIINO_BOARD=mini VARIANT=uno F_CPU=16000000

# Mega 2560 
# usb processor
	$(MAKE) -C src/uno/usb OBJDIR=obj/16u22560 MCU=atmega16u2 ARDWIINO_BOARD=mega2560
	$(MAKE) -C src/uno/usb OBJDIR=obj/8u22560 MCU=at90usb82 ARDWIINO_BOARD=mega2560
# usb processor - multi adaptor
	$(MAKE) -C src/uno/usb OBJDIR=obj/multi/16u22560 MCU=atmega16u2 ARDWIINO_BOARD=mega2560 MULTI_ADAPTOR=-DMULTI_ADAPTOR
	$(MAKE) -C src/uno/usb OBJDIR=obj/multi/8u22560 MCU=at90usb82 ARDWIINO_BOARD=mega2560 MULTI_ADAPTOR=-DMULTI_ADAPTOR
# usb processor - usbserial firmware
	$(MAKE) -C submodules/arduino-usbserial-bootloader OBJDIR=obj/16u22560 MCU=atmega16u2 ARDUINO_MODEL_PID=${MEGA2560_PID} A_BOARD=mega2560
	$(MAKE) -C submodules/arduino-usbserial-bootloader OBJDIR=obj/8u22560 MCU=at90usb82 ARDUINO_MODEL_PID=${MEGA2560_PID} A_BOARD=mega2560
# main processor
	$(MAKE) -C src/uno/main OBJDIR=obj/2560 MCU=atmega2560 ARDWIINO_BOARD=mega2560 VARIANT=mega
# main processor - rf transmitter
	$(MAKE) -C src/uno/rf OBJDIR=obj/2560 MCU=atmega2560 ARDWIINO_BOARD=mega2560 VARIANT=mega

# Mega ADK
# usb processor
	$(MAKE) -C src/uno/usb OBJDIR=obj/16u2adk MCU=atmega16u2 ARDWIINO_BOARD=megaadk
	$(MAKE) -C src/uno/usb OBJDIR=obj/8u2adk MCU=at90usb82 ARDWIINO_BOARD=megaadk
# usb processor - multi adaptor
	$(MAKE) -C src/uno/usb OBJDIR=obj/multi/16u2adk MCU=atmega16u2 ARDWIINO_BOARD=megaadk MULTI_ADAPTOR=-DMULTI_ADAPTOR
	$(MAKE) -C src/uno/usb OBJDIR=obj/multi/8u2adk MCU=at90usb82 ARDWIINO_BOARD=megaadk MULTI_ADAPTOR=-DMULTI_ADAPTOR
# usb processor - usbserial firmware
	$(MAKE) -C submodules/arduino-usbserial-bootloader OBJDIR=obj/16u2adk MCU=atmega16u2 ARDUINO_MODEL_PID=${MEGAADK_PID} A_BOARD=megaadk
	$(MAKE) -C submodules/arduino-usbserial-bootloader OBJDIR=obj/8u2adk MCU=at90usb82 ARDUINO_MODEL_PID=${MEGAADK_PID} A_BOARD=megaadk
# main processor
	$(MAKE) -C src/uno/main OBJDIR=obj/adk MCU=atmega2560 ARDWIINO_BOARD=mega2560 VARIANT=mega

# main processor - rf transmitter
	$(MAKE) -C src/uno/rf OBJDIR=obj/adk MCU=atmega2560 ARDWIINO_BOARD=mega2560 VARIANT=mega


# Arduino leonardo
	$(MAKE) -C src/micro/main OBJDIR=obj/leo/16 ARDWIINO_BOARD=leonardo F_CPU=16000000 F_USB=16000000
	$(MAKE) -C src/micro/main OBJDIR=obj/leo/8 ARDWIINO_BOARD=leonardo F_CPU=8000000 F_USB=8000000
# multi adaptor
	$(MAKE) -C src/micro/main OBJDIR=obj/leo/multi/16 ARDWIINO_BOARD=leonardo F_CPU=16000000 F_USB=16000000 MULTI_ADAPTOR=-DMULTI_ADAPTOR
	$(MAKE) -C src/micro/main OBJDIR=obj/leo/multi/8 ARDWIINO_BOARD=leonardo F_CPU=8000000 F_USB=8000000 MULTI_ADAPTOR=-DMULTI_ADAPTOR
# rf transmitter
	$(MAKE) -C src/micro/rf OBJDIR=obj/leo/multi/16 ARDWIINO_BOARD=leonardo F_CPU=16000000 F_USB=16000000
	$(MAKE) -C src/micro/rf OBJDIR=obj/leo/multi/8 ARDWIINO_BOARD=leonardo F_CPU=8000000 F_USB=8000000


# Arduino Micro
	$(MAKE) -C src/micro/main OBJDIR=obj/a-micro/16 ARDWIINO_BOARD=a-micro F_CPU=16000000 F_USB=16000000
# multi adaptor
	$(MAKE) -C src/micro/main OBJDIR=obj/a-micro/multi/16 ARDWIINO_BOARD=a-micro F_CPU=16000000 F_USB=16000000 MULTI_ADAPTOR=-DMULTI_ADAPTOR
# rf transmitter
	$(MAKE) -C src/micro/rf OBJDIR=obj/a-micro/rf/16 ARDWIINO_BOARD=a-micro F_CPU=16000000 F_USB=16000000

# Arduino Pro Micro
	$(MAKE) -C src/micro/main OBJDIR=obj/micro/16 ARDWIINO_BOARD=micro F_CPU=16000000 F_USB=16000000
	$(MAKE) -C src/micro/main OBJDIR=obj/micro/8 ARDWIINO_BOARD=micro F_CPU=8000000 F_USB=8000000
# multi adaptor
	$(MAKE) -C src/micro/main OBJDIR=obj/micro/multi/16 ARDWIINO_BOARD=micro F_CPU=16000000 F_USB=16000000 MULTI_ADAPTOR=-DMULTI_ADAPTOR
	$(MAKE) -C src/micro/main OBJDIR=obj/micro/multi/8 ARDWIINO_BOARD=micro F_CPU=8000000 F_USB=8000000 MULTI_ADAPTOR=-DMULTI_ADAPTOR
# rf transmitter
	$(MAKE) -C src/micro/rf OBJDIR=obj/micro/rf/16 ARDWIINO_BOARD=micro F_CPU=16000000 F_USB=16000000
	$(MAKE) -C src/micro/rf OBJDIR=obj/micro/rf/8 ARDWIINO_BOARD=micro F_CPU=8000000 F_USB=8000000
	
	mkdir -p output
	cp -rfv src/uno/usb/bin/*.hex output/
	cp -rfv src/uno/main/bin/*.hex output/
	cp -rfv src/uno/rf/bin/*.hex output/
	cp -rfv src/mini/rf/bin/*.hex output/
	cp -rfv src/micro/main/bin/*.hex output/
	cp -rfv src/micro/rf/bin/*.hex output/
	cp -rfv submodules/arduino-usbserial-bootloader/out/*.hex output/

	rm -rfv src/uno/usb/{bin,obj}
	rm -rfv src/uno/main/{bin,obj}
	rm -rfv src/uno/rf/{bin,obj}
	rm -rfv src/mini/rf/{bin,obj}
	rm -rfv src/micro/main/{bin,obj}
	rm -rfv src/micro/rf/{bin,obj}
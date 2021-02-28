UNOMCU          = atmega16u2
UNOMCU8          = at90usb82
UNO_PID			=0x0001
MEGA2560_PID	=0x0010
MEGAADK_PID		=0x003f
all:

micro:
	$(MAKE) -C src/avr/micro/main
	-stty -F /dev/ttyACM0 1200 || scripts/bootloader.py
	sleep 2
	$(MAKE) -C src/avr/micro/main avrdude

micro_rf:
	$(MAKE) -C src/avr/micro/rf
	-stty -F /dev/ttyACM0 1200 || scripts/bootloader.py
	sleep 2
	$(MAKE) -C src/avr/micro/rf avrdude

gdb:
	-avarice -j /dev/ttyUSB0 -P atmega32 :4242 -r -R & avr-gdb ./src/avr/micro/main/bin/ardwiino-micro-atmega32u4-8000000.elf

uno-8:
	$(MAKE) -C src/avr/uno
	sleep 0.5
	scripts/bootloader.py || true
	sleep 1
	dfu-programmer $(UNOMCU8) erase || true
	dfu-programmer $(UNOMCU8) flash build/firmware/ardwiino-uno-usb-$(UNOMCU8)-16000000-usbserial.hex
	dfu-programmer $(UNOMCU8) launch
	sleep 1
	$(MAKE) -C src/avr/uno/main avrdude
	sleep 1
	stty -F /dev/ttyACM0 1200
	sleep 1
	$(MAKE) -C src/avr/uno/usb dfu MCU=$(UNOMCU8)

uno:
	# $(MAKE) -C src/avr/uno/
	sleep 0.5
	scripts/bootloader.py || true
	sleep 1
	dfu-programmer $(UNOMCU) erase || true
	dfu-programmer $(UNOMCU) flash build/firmware/ardwiino-uno-usb-$(UNOMCU)-16000000-usbserial.hex
	dfu-programmer $(UNOMCU) launch
	sleep 2
	$(MAKE) -C src/avr/uno/main avrdude PORT=/dev/ttyACM0
	sleep 2
	stty -F /dev/ttyACM0 1200
	sleep 1
	dfu-programmer $(UNOMCU) erase || true
	$(MAKE) -C src/avr/uno/usb dfu MCU=$(UNOMCU)

uno-rftx:
	$(MAKE) -C src/avr/uno/rf
	$(MAKE) -C src/avr/uno/rf avrdude
	$(MAKE) -C src/avr/uno/rf_rx
	$(MAKE) -C src/avr/uno/rf_rx avrdude

mini-rf:
	# $(MAKE) -C src/avr/micro/main
	# -stty -F /dev/ttyACM0 1200 || scripts/bootloader.py
	# sleep 1
	# $(MAKE) -C src/avr/micro/main avrdude
	$(MAKE) -C src/avr/uno/rf
	$(MAKE) -C src/avr/uno/rf avrdude PORT=/dev/ttyUSB0
unomini-rf:
	$(MAKE) -C src/avr/uno
	sleep 0.5
	scripts/bootloader.py || true
	sleep 1
	dfu-programmer $(UNOMCU) erase || true
	dfu-programmer $(UNOMCU) flash output/ardwiino-uno-usb-$(UNOMCU)-16000000-usbserial.hex
	dfu-programmer $(UNOMCU) launch
	sleep 1
	$(MAKE) -C src/avr/uno/main avrdude
	sleep 1
	stty -F /dev/ttyACM0 1200
	sleep 1
	$(MAKE) -C src/avr/uno/usb dfu MCU=$(UNOMCU)
	# sleep 1
	# stty -F /dev/ttyACM1 1200
	# sleep 1
	# $(MAKE) -C src/avr/uno/usb dfu MCU=$(UNOMCU)
	$(MAKE) -C src/avr/mini/rf
	$(MAKE) -C src/avr/mini/rf avrdude PORT=/dev/ttyUSB0
uno-rf:
	$(MAKE) -C src/avr/uno
	sleep 0.5
	-scripts/bootloader.py
	-stty -F /dev/ttyACM1 1200
	sleep 1
	dfu-programmer $(UNOMCU) erase || true
	dfu-programmer $(UNOMCU) flash output/ardwiino-uno-usb-$(UNOMCU)-16000000-usbserial.hex
	dfu-programmer $(UNOMCU) launch
	sleep 2
	$(MAKE) -C src/avr/uno/main avrdude PORT=/dev/ttyACM1
	sleep 1
	stty -F /dev/ttyACM1 1200
	sleep 1
	$(MAKE) -C src/avr/uno/usb dfu MCU=$(UNOMCU)
	$(MAKE) -C src/avr/uno/rf
	$(MAKE) -C src/avr/uno/rf avrdude PORT=/dev/ttyACM0
	
uno-rf2:
	$(MAKE) -C src/avr/uno/rfRecTest
	$(MAKE) -C src/avr/uno/rfRecTest avrdude
	$(MAKE) -C src/avr/uno/rf
	$(MAKE) -C src/avr/uno/rf avrdude

uno-rf3:
	$(MAKE) -C src/avr/uno
	$(MAKE) -C src/avr/uno/main avrdude PORT=/dev/ttyACM1
	# sleep 1
	# stty -F /dev/ttyACM1 1200
	# sleep 1
	# $(MAKE) -C src/avr/uno/usb dfu MCU=$(UNOMCU)
	$(MAKE) -C src/avr/uno/rf
	$(MAKE) -C src/avr/uno/rf avrdude PORT=/dev/ttyACM0

clean:
	$(MAKE) -C src/avr/micro/main clean
	$(MAKE) -C src/avr/uno clean

build:
	$(MAKE) -C src/avr/micro/main
	$(MAKE) -C src/avr/uno
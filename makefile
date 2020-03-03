all:

micro:
	$(MAKE) -C src/micro
	stty -F /dev/ttyACM0 1200
	sleep 1
	$(MAKE) -C src/micro avrdude

uno:
	$(MAKE) -C src/uno
	sleep 0.5
	$(MAKE) -C src/uno upload

clean:
	$(MAKE) -C src/micro clean
	$(MAKE) -C src/uno clean

build:
	$(MAKE) -C src/micro
	$(MAKE) -C src/uno

build-all:
	$(MAKE) -C src/uno/usb OBJDIR=obj/16u2
	$(MAKE) -C src/uno/usb OBJDIR=obj/8u2 MCU=at90usb82
	$(MAKE) -C src/uno/main
	$(MAKE) -C src/micro OBJDIR=obj/micro/16 ARDWIINO_BOARD=micro F_CPU=16000000 F_USB=16000000
	$(MAKE) -C src/micro OBJDIR=obj/micro/8 ARDWIINO_BOARD=micro F_CPU=8000000 F_USB=8000000
	$(MAKE) -C src/micro OBJDIR=obj/leo/16 ARDWIINO_BOARD=leonardo F_CPU=16000000 F_USB=16000000
	$(MAKE) -C src/micro OBJDIR=obj/leo/8 ARDWIINO_BOARD=leonardo F_CPU=8000000 F_USB=8000000
	mkdir -p output
	cp -rfv src/uno/usb/bin/*.hex output/
	cp -rfv src/uno/main/bin/*.hex output/
	cp -rfv src/micro/bin/*.hex output/
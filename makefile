all:

micro:
	$(MAKE) -C src/micro
	echo b > /dev/ttyACM0
	sleep 0.5
	$(MAKE) -C src/micro avrdude

micro-ee:
	$(MAKE) -C src/micro
	echo b > /dev/ttyACM0
	sleep 0.5
	$(MAKE) -C src/micro avrdude-ee

uno:
	$(MAKE) -C src/uno
	sleep 0.5
	$(MAKE) -C src/uno upload

uno-ee:
	$(MAKE) -C src/uno
	sleep 0.5
	$(MAKE) -C src/uno upload-ee

clean:
	$(MAKE) -C src/micro clean
	$(MAKE) -C src/uno clean

build:
	$(MAKE) -C src/micro
	$(MAKE) -C src/uno

build-all:
	rm -rf output
	mkdir output
	$(MAKE) clean
	MCU=atmega8u2 $(MAKE) build
	cp src/uno/usb/bin/*.hex output/
	cp src/uno/usb/bin/*.eep output/
	F_CPU=16000000 $(MAKE) build
	F_CPU=8000000 $(MAKE) build
	cp src/micro/bin/*.hex output/
	cp src/micro/bin/*.eep output/
	cp src/uno/main/bin/*.hex output/
	cp src/uno/main/bin/*.eep output/
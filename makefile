all:

micro:
	$(MAKE) -C src/micro
	stty -F /dev/ttyACM0 1200
	sleep 1
	$(MAKE) -C src/micro avrdude

micro-ee:
	$(MAKE) -C src/micro
	stty -F /dev/ttyACM0 1200
	sleep 1
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
	$(MAKE) -C src/uno/usb clean
	$(MAKE) -C src/uno/usb
	cp src/uno/usb/bin/*.{hex,eep} output/
	$(MAKE) -C src/uno/main clean
	$(MAKE) -C src/uno/main
	cp src/uno/main/bin/*.{hex,eep} output/
	$(MAKE) -C src/micro clean ARDWIINO_BOARD=micro F_CPU=16000000 F_USB=16000000
	$(MAKE) -C src/micro ARDWIINO_BOARD=micro F_CPU=16000000 F_USB=16000000
	cp src/micro/bin/*.{hex,eep} output/
	$(MAKE) -C src/micro clean ARDWIINO_BOARD=micro F_CPU=8000000 F_USB=8000000
	$(MAKE) -C src/micro ARDWIINO_BOARD=micro F_CPU=8000000 F_USB=8000000
	cp src/micro/bin/*.{hex,eep} output/
	$(MAKE) -C src/micro clean ARDWIINO_BOARD=leonardo F_CPU=16000000 F_USB=16000000
	$(MAKE) -C src/micro ARDWIINO_BOARD=leonardo F_CPU=16000000 F_USB=16000000
	cp src/micro/bin/*.{hex,eep} output/
	$(MAKE) -C src/micro clean ARDWIINO_BOARD=leonardo F_CPU=8000000 F_USB=8000000
	$(MAKE) -C src/micro ARDWIINO_BOARD=leonardo F_CPU=8000000 F_USB=8000000
	cp src/micro/bin/*.{hex,eep} output/
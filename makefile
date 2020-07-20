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
# Arduino uno
# usb processor
	$(MAKE) -C src/uno/usb OBJDIR=obj/16u2 MCU=atmega16u2 ARDWIINO_BOARD=uno
	$(MAKE) -C src/uno/usb OBJDIR=obj/8u2 MCU=at90usb82 ARDWIINO_BOARD=uno
# usb processor - multi adaptor
	$(MAKE) -C src/uno/usb OBJDIR=obj/multi/16u2 MCU=atmega16u2 ARDWIINO_BOARD=uno-multi MULTI_ADAPTOR=-DMULTI_ADAPTOR
	$(MAKE) -C src/uno/usb OBJDIR=obj/multi/8u2 MCU=at90usb82 ARDWIINO_BOARD=uno-multi MULTI_ADAPTOR=-DMULTI_ADAPTOR
# main processor
	$(MAKE) -C src/uno/main OBJDIR=obj/uno MCU=atmega328p ARDWIINO_BOARD=uno


# Mega 2560 
# usb processor
	$(MAKE) -C src/uno/usb OBJDIR=obj/16u22560 MCU=atmega16u2 ARDWIINO_BOARD=mega2560
	$(MAKE) -C src/uno/usb OBJDIR=obj/8u22560 MCU=at90usb82 ARDWIINO_BOARD=mega2560
# usb processor - multi adaptor
	$(MAKE) -C src/uno/usb OBJDIR=obj/multi/16u22560 MCU=atmega16u2 ARDWIINO_BOARD=mega2560-multi MULTI_ADAPTOR=-DMULTI_ADAPTOR
	$(MAKE) -C src/uno/usb OBJDIR=obj/multi/8u22560 MCU=at90usb82 ARDWIINO_BOARD=mega2560-multi MULTI_ADAPTOR=-DMULTI_ADAPTOR
# main processor
	$(MAKE) -C src/uno/main OBJDIR=obj/2560 MCU=atmega2560 ARDWIINO_BOARD=mega2560


# Mega ADK
# usb processor
	$(MAKE) -C src/uno/usb OBJDIR=obj/16u2adk MCU=atmega16u2 ARDWIINO_BOARD=megaadk
	$(MAKE) -C src/uno/usb OBJDIR=obj/8u2adk MCU=at90usb82 ARDWIINO_BOARD=megaadk
# usb processor - multi adaptor
	$(MAKE) -C src/uno/usb OBJDIR=obj/multi/16u2adk MCU=atmega16u2 ARDWIINO_BOARD=megaadk-multi MULTI_ADAPTOR=-DMULTI_ADAPTOR
	$(MAKE) -C src/uno/usb OBJDIR=obj/multi/8u2adk MCU=at90usb82 ARDWIINO_BOARD=megaadk-multi MULTI_ADAPTOR=-DMULTI_ADAPTOR
# main processor
	$(MAKE) -C src/uno/main OBJDIR=obj/adk MCU=atmega2560 ARDWIINO_BOARD=mega2560


# Arduino leonardo
	$(MAKE) -C src/micro OBJDIR=obj/leo/16 ARDWIINO_BOARD=leonardo F_CPU=16000000 F_USB=16000000
	$(MAKE) -C src/micro OBJDIR=obj/leo/8 ARDWIINO_BOARD=leonardo F_CPU=8000000 F_USB=8000000
# multi adaptor
	$(MAKE) -C src/micro OBJDIR=obj/leo/multi/16 ARDWIINO_BOARD=leonardo-multi F_CPU=16000000 F_USB=16000000 MULTI_ADAPTOR=-DMULTI_ADAPTOR
	$(MAKE) -C src/micro OBJDIR=obj/leo/multi/8 ARDWIINO_BOARD=leonardo-multi F_CPU=8000000 F_USB=8000000 MULTI_ADAPTOR=-DMULTI_ADAPTOR


# Arduino Micro
	$(MAKE) -C src/micro OBJDIR=obj/a-micro/16 ARDWIINO_BOARD=a-micro F_CPU=16000000 F_USB=16000000
# multi adaptor
	$(MAKE) -C src/micro OBJDIR=obj/a-micro/multi/16 ARDWIINO_BOARD=a-micro-multi F_CPU=16000000 F_USB=16000000 MULTI_ADAPTOR=-DMULTI_ADAPTOR


# Arduino Pro Micro
	$(MAKE) -C src/micro OBJDIR=obj/micro/16 ARDWIINO_BOARD=micro F_CPU=16000000 F_USB=16000000
	$(MAKE) -C src/micro OBJDIR=obj/micro/8 ARDWIINO_BOARD=micro F_CPU=8000000 F_USB=8000000
# multi adaptor
	$(MAKE) -C src/micro OBJDIR=obj/micro/multi/16 ARDWIINO_BOARD=micro-multi F_CPU=16000000 F_USB=16000000 MULTI_ADAPTOR=-DMULTI_ADAPTOR
	$(MAKE) -C src/micro OBJDIR=obj/micro/multi/8 ARDWIINO_BOARD=micro-multi F_CPU=8000000 F_USB=8000000 MULTI_ADAPTOR=-DMULTI_ADAPTOR


	mkdir -p output
	cp -rfv src/uno/usb/bin/*.hex output/
	cp -rfv src/uno/main/bin/*.hex output/
	cp -rfv src/micro/bin/*.hex output/
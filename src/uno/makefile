all:

upload: 
	@echo Uploading all elements.
	@echo
	echo q > /dev/ttyACM0
	$(MAKE) -C main avrdude
	sleep 1
	echo b > /dev/ttyACM0
	sleep 0.5
	$(MAKE) -C usb dfu
	@echo
	@echo Upload operation complete.

upload-ee:
	@echo Uploading eeprom to usb.
	@echo
	echo b > /dev/ttyACM0
	sleep 0.5
	$(MAKE) -C usb avrdude-ee
	@echo
	@echo Upload operation complete.

clean:
	@echo Cleaning all elements.
	@echo
	$(MAKE) -C main clean
	$(MAKE) -C usb clean
	@echo
	@echo Clean operation complete.

%:
	@echo Executing \"make $@\" on all elements.
	@echo
	$(MAKE) -C main
	$(MAKE) -C usb
	@echo
	@echo \"Make $@\" operation complete.
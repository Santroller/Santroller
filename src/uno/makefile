all:

upload:
	@echo Uploading all elements.
	@echo
	$(MAKE) -C main avrdude
	$(MAKE) -C usb avrdude
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
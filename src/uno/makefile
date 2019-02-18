all:

upload:
	make
	@echo Uploading all elements.
	@echo
	$(MAKE) -C main upload
	$(MAKE) -C usb avrdude
	@echo
	@echo Upload operation complete.

%:
	@echo Executing \"make $@\" on all elements.
	@echo
	$(MAKE) -C main
	$(MAKE) -C usb
	@echo
	@echo \"Make $@\" operation complete.
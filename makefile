all:

upload:
	$(MAKE) -C src avrdude

%:
	$(MAKE) -C src $@
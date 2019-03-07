all:

micro:
	$(MAKE) -C src/micro avrdude

uno:
	$(MAKE) -C src/uno upload

clean:
	$(MAKE) -C src/micro clean
	$(MAKE) -C src/uno clean

build:
	$(MAKE) -C src/micro
	$(MAKE) -C src/uno
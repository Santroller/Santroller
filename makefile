all:

micro:
	$(MAKE) -C src/micro
	./upload.py
	sleep 0.5
	$(MAKE) -C src/micro avrdude

micro-ee:
	$(MAKE) -C src/micro
	./upload.py
	sleep 0.5
	$(MAKE) -C src/micro avrdude-ee

uno:
	$(MAKE) -C src/uno
	./upload.py
	sleep 0.5
	$(MAKE) -C src/uno upload

uno-ee:
	$(MAKE) -C src/uno
	./upload.py
	sleep 0.5
	$(MAKE) -C src/uno upload-ee

clean:
	$(MAKE) -C src/micro clean
	$(MAKE) -C src/uno clean

build:
	$(MAKE) -C src/micro
	$(MAKE) -C src/uno
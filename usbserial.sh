dfu-programmer atmega16u2 erase 
sleep 1
dfu-programmer atmega16u2 flash ardwiino-uno-usb-atmega16u2-16000000-usbserial.hex
sleep 1
dfu-programmer atmega16u2 launch --no-reset
sleep 1
dfu-programmer atmega16u2 launch --no-reset
sleep 1
cd build
make uno-main-atmega328p-16000000-avrdude
sleep 1
dfu-programmer atmega16u2 erase 
sleep 1
make uno-usb-atmega16u2-16000000-dfu
cd ..
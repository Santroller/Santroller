dfu-programmer atmega16u2 erase 
sleep 2
dfu-programmer atmega16u2 flash ardwiino-uno-usb-atmega16u2-16000000-usbserial.hex
sleep 2
dfu-programmer atmega16u2 launch --no-reset
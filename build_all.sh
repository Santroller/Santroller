#!/bin/bash
rm -r output
mkdir output
make clean
F_CPU=16000000 make build
F_CPU=8000000 make build
cp src/micro/bin/*.hex output/
cp src/uno/main/bin/*.hex output/
MCU=atmega8u2 make build
cp src/uno/usb/bin/*.hex output/
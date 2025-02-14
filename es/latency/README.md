---
sort: 3
---
# Latency Information ES
For a game controller, a controller with less latency will respond faster when you press a button or move an axis. 

I use the [usblag](https://gitlab.com/loic.petit/usblag) program to measure latency. With this program running on an Arduino Uno with a USB Host Shield, it can simulate a button press on a controller by grounding the pin the button is connected to, which the game controller will see as a button press. It then starts a timer and calculates how long it takes for the game controller to send that button press over USB to the USB Host Shield. We can then take 1000 samples over a random time period, and analyse this data to work out information about the latency of a controller.
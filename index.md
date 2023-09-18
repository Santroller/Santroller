---
layout: default
---
![Santroller](assets/images/logo.png){: width="250" }


The Santroller platform provides the ability to program micro controllers that emulate various rhythm game controllers for computers and various consoles.
This is done using the Santroller Configurator, which can program various microcontrollers to emulate different controllers.


# Gallery

[![home screen](assets/images/screenshots/home.png){: height="215" }](assets/images/screenshots/home.png) 
[![main screen](assets/images/screenshots/main.png){: height="215" }](assets/images/screenshots/main.png)
[![main screen with everything](assets/images/screenshots/combined.png){: height="215" }](assets/images/screenshots/combined.png)
[![alt text](assets/images/adaptor.jpg){: height="215" }](assets/images/adaptor.jpg)
[![alt text](assets/images/adaptor-ps2.jpg){: height="215" }](assets/images/adaptor-ps2.jpg)
[![alt text](assets/images/direct.jpg){: height="215" }](assets/images/direct.jpg)
[![alt text](assets/images/inline-led.jpg){: height="215" }](assets/images/inline-led.jpg)
# Features
* Support for various consoles, including PS2/3/4/5, Wii, Switch, Xbox 360 and Xbox One / Series
  * PS2 requires a [nightly build of OPL](https://santroller.tangentmc.net/console_guides/ps2.html) for most games, some of the later games do support PS3 instruments nativly though.
  * PS3 works nativly with no additional requirements
  * PS4 and PS5 will use PS3 mode and work with no additional requirements, except for gamepad and guitar hero live guitar mode which requires an [authentication](https://santroller.tangentmc.net/console_guides/authentication.html) controller
  * Xbox 360 either requires the use of an [authentication](https://santroller.tangentmc.net/console_guides/authentication.html) controller, or to use [UsbdSecPatch](https://santroller.tangentmc.net/console_guides/authentication.html#usbdsecpatch) to bypass authentication on a RGHed console.
  * Xbox one requires an [authentication](https://santroller.tangentmc.net/console_guides/authentication.html) controller
  * Switch works nativly with no additional requirements
  * Rock Band for wii supports usb instruments nativly with no extra requirements
  * Guitar Hero for wii requires the use of [fakemote](https://santroller.tangentmc.net/console_guides/wii.html), with a modded console, as these expect wii extensions not usb instruments.
* Support for all major operating systems (Windows, macOS, linux)
* Controllers automatically detect what they are plugged into, and reconfigure themselves for that device.
* Support for various microcontrollers
  * Pi Pico (This supports the most features and is recommended as it is easier)
  * Arduino Pro Micro (3.3v)
  * Arduino Pro Micro (5v)
  * Arduino Leonardo
  * Arduino Micro
  * Arduino Uno R1/2/3
  * Arduino Mega
* Support for input from Wii extensions
* Support for input from USB devices
  * This is also used for controller authentication, as this allows for plugging in an offical controller for a console to proxy authentication checks, to allow use on a unmodified console.
* Support for direct inputs
* Support for bluetooth
  * For PC, this works directly, and you can connect to a bluetooth configured controller
  * For consoles, you can build a bluetooth receiver that connects to a bluetooth configured controller, and allows for wireless play
* Support for inputs from another pi pico
  * This can be used to put LEDs in a DJ Hero turntable, or to bypass the GH5 neck's microcontroller but keep it removable.
* Ability to emulate the following controllers
  * Guitar Hero Guitars
  * Rock Band Guitars
  * Guitar Hero Drums
  * Rock Band Drums
  * Rock Band Pro Drums
  * Guitar Hero Live Guitars
  * DJ Hero Turntables
  * Rock Band Stage Kit
  * Standard Gamepad
* Button combos
  * This allows for features like emulating a home button by pressing start + select
* Various LED options, including seperate states for pressed and released, and reaction to game events when possible.
* Reactive configuration tool, to make calibration as easy as possible.

```note
Avoid using USB 2.0 hubs when using these controllers, they poll so quickly that multiple guitars on a single hub will be slowed down, and some hubs will even cause issues with the controller being detected. However, USB 3.0 hubs are okay, and I have previously used multiple guitars on a single USB 3.0 hub with no issues.
```

# How to build an adaptor or controller
Pick a guide from the left to learn how to build a controller or adaptor.

# Need Help? Something not working?
Join the discord, and someone should be able to help you with your problem.

# Download link
Click [Here](https://github.com/sanjay900/SantrollerConfigurator/releases/latest) to download the configuration tool.

# Using this tool commercially
If you would look to sell devices running the Santroller firmware, click the [following link](https://santroller.tangentmc.net/tool/commercial_use.html) to learn more about the commercial version of this platform.
The standard version of this tool can NOT be used commercially, and the commercial version of this platform has extra features that are designed specifically for users who are selling products running on this platform.


# Donate Link
I have put several years of development time into this project, if you like it feel free to support this project by clicking the below link:
{% include sponser.html %}

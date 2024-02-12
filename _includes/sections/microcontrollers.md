{% details Microcontroller information %}
  * Pi Pico 
    * This micro controller was created by the Raspberry Pi foundation, so it means that buying one supports them instead of random companies cloning Arduinos. They are considerably more powerful than any of the other supported Arduinos. The Pi Pico supports a few extra features, such as USB Input (which allows for XB1 / Series compatability), Peripheral microcontrollers and the GHWT tap bar. Note that the Pi Pico runs at 3.3v, so it can just be used as is for PS2 and Wii adapters. Pi Picos also tend to be quite cheap and often have the advantage that they can often be brought at local stores. With the currrent firmware, picos work much much better than arduinos if your goal is to create adaptors, but they also work better for direct wiring too. There are many boards based on the Pi Pico as well, and all of these work just fine with the firmware.
  * Sparkfun Pro Micro (5v)
    * The 5V Pro Micro will work okay for direct wiring, but being that it runs at 5V, it will require voltage conversion to build Wii adapters and PS2 adapters and turntables.
  * Sparkfun Pro Micro (3.3v)
    * If you want to build an adapter for a Wii or PS2 guitar, then this will be easier to use than any of the 5v microcontrollers. Due to the lower voltage, these do run at half the speed of the 5v variants, a. Clones of the Pro Micro are quite cheap but will need to be purchased from ebay or aliexpress, real Pro Micros are quite expensive but there isn't really much of a difference. 3.3V arduinos will poll a wii guitar slightly slower than a 5V arduino but in practise this does not end up mattering as there are a lot of other delays necessary for communicating with a wii guitar.
  * Arduino Micro
    * This is essentially the same thing as a 5v Pro Micro, however these are officially made by Arduino. These are often a bit more expensive as they aren't really cloned.
  * Arduino Leonardo
    * This is essentially a 5v Pro Micro with the layout of a Arduino Uno, so you get more pins but it is also much larger. You can get clones of these, but they are still more expensive than Pro Micros or picos.
  * Arduino Uno (r1/r2/r3)
    * These micro controllers are actually two micro controllers in one, and they work in tandem to provide a working controller. This has its disadvantages, as code needs to keep these controllers in sync, and this can result in issues if a bad configuration is programmed, and generally results in requiring more complicated and optimised code to work. Unos do still get 1000hz, but I would recommend against them if you are buying a new Arduino. Note that some clone Arduino Unos are actually missing the second micro controller, and these ones will NOT work at all. If you see an Arduino Uno listing that mentions "ch340g" or something along those lines do not purchase it. Due to this, they are harder to purchase and since they require more parts, they are more expensive than a Pro Micro or Pi Pico.
  * Arduino Mega 2560
    * These are in the same situation as the Uno, however the main micro controller has a lot more pins. These do also end up being rather expensive due to the sheer amount of parts that are needed to make one.
  * Arduino Uno r4
    * This is the newest entrant to the Arduino Uno line, and it is a totally different microcontroller and hence it is not supported.
  * Arduino Mini or Nano or Pro Mini or Pro Nano
    * These do NOT work, as they are essentially Unos that lack the second micro controller that allow for custom USB device emulation.
  * ESP32
    * Currently this is not supported due to the base ESP32 not fully supporting USB. There are some ESP32 models that have USB support, but these end up being more expensive than the Pi Pico, and thus it does not make sense to support these.
{% enddetails %}

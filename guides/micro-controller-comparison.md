# Micro controller comparison

Below is a comparison of the various micro controllers that are supported by the configuration tool

## Pi Pico 
This micro controller was created by the Raspberry Pi foundation, so it means that buying one supports them instead of random companies cloning Arduinos. They are considerably more powerful than any of the other supported Arduinos, and as a result they will be getting some new features in the future, solely due to them supporting more features. Note that the Pi Pico runs at 3.3v, so it can also be used for adapters. Pi Picos also tend to be quite cheap and often have the advantage that they can often be brought at local stores. With the currrent firmware, picos work much much better than arduinos if your goal is to create adaptors, but they also just tend to work quite well for direct wiring too, and at this point I recommend them over pro micros.

## Sparkfun Pro Micro (5v)
If you are building a guitar and planning on directly wiring all the buttons to it, than this is probably the best of the Arduinos, as it is small and runs at a faster speed over the 3.3v variants. These can still be used to build adapters but will require parts for stepping the voltage down from 5v to 3.3v. Clones of the Pro Micro are quite cheap but will need to be purchased from ebay or aliexpress, real Pro Micros are quite expensive but unnecessary. These arduinos are not quite powerful enough to poll a wii guitar at full speed, and neither are any of the other arduinos. 

## Sparkfun Pro Micro (3.3v)
If you want to build an adapter for a Wii or PS2 guitar, than this is the easiest Arduino to use. Due to the lower voltage, these do run at half the speed of the 5v variants, a. Clones of the Pro Micro are quite cheap but will need to be purchased from ebay or aliexpress, real Pro Micros are quite expensive but there isn't really much of a difference. 3.3V arduinos will poll a wii guitar slightly slower than a 5V arduino but in practise this does not end up mattering as there are a lot of other delays necessary for communicating with a wii guitar.

## Arduino Micro
This is essentially the same thing as a 5v pro micro, however these are officially made by Arduino. These are often a bit more expensive as they aren't really cloned.

## Arduino Leonardo
This is essentially a 5v pro micro with the layout of a Arduino UNO, so you get more pins but it is also much larger. You can get clones of these, but they are still more expensive than pro micros or picos.

## Arduino UNO
These micro controllers are actually two micro controllers in one, and they work in tandem to provide a working controller. This has its disadvantages, as code needs to keep these controllers in sync, and this can result in issues if a bad configuration is programmed, and generally results in requiring more complicated and optimised code to work. UNOs do still get 1000hz, but I would recommend against them if you are buying a new Arduino. Note that some clone Arduino UNOs are actually missing the second micro controller, and these ones will NOT work at all. If you see an Arduino UNO listing that mentions "ch340g" or something along those lines do not purchase it. Due to this, they are harder to purchase and since they require more parts, they are more expensive than a Pro Micro or Pi Pico.

## Arduino Mega 2560
These are in the same situation as the UNO, however the main micro controller has a lot more pins. These do also end up being rather expensive due to the sheer amount of parts that are needed to make one.

## Arduino Mini
These do NOT work for USB guitar controllers, as they are essentially UNOs that lack the second micro controller that makes this all work. They are inexpensive though, and in the future once the RF code is rewritten and more stable, they will be useful for anybody wanting to build wireless guitars, as they use a very tiny amount of power which results in a higher battery life. In this case the the ability to communicate over USB is not necessary as they would communicate with a second micro controller over RF, and only that micro controller would need USB.

---
sort: 5
---
# Building an Adaptor for a Wii Controller
## You will need
* A microcontroller from the list in the [following guide](https://santroller.tangentmc.net/wiring_guides/general.html)

* A basic tilt switch (if you want tilt) (sometimes called a Mercury switch or ball tilt switch). 
  * It is recommended to use two, as this can help avoid accidental activations (this is what is done in some offical guitars)  
  * I know some people use the SW-520D (which is a ball tilt switch), I myself just use some mercury tilt sensors that my local electronics shop sells.
  * Make sure that you get a basic tilt sensor and not one with additional electronics, for example, this type of sensor is correct 
    
  [![Basic](../assets/images/s-l500-basic.jpg){: width="250" }](../assets/images/s-l500.jpg)

```danger
  You do not want to get this type of sensor, as it does not work.
    
  [![Basic](../assets/images/s-l500.png){: width="250" }](../assets/images/s-l500.png)
```
```danger
For anyone wanting to use tilt, the only supported method is using digital tilt switches. Support for the MPU-6050 and analog tilt sensors is now deprecated and will be removed from the firmware in a future update. They are causing a lot of issues for porting this code between various microcontrollers, and take up too much space in the firmware.
```
* A Wii extension breakout board or an extension cable, such as [![this](https://www.adafruit.com/product/4836){: width="250" }](https://www.adafruit.com/product/4836). You can also choose to cut the end of the extension and solder your own cables on as well if you perfer.
* If your wii extension breakout does not support 3.3v input, and you are using a 5v pro micro, you will need a 3.3v voltage regulator. The breakout listed above does however support either voltage so this is not required for that breakout.
* Some Wire
* A Soldering Iron

```note
If you are unsure what microcontroller you want to use, click [![here](https://sanjay900.github.io/guitar-configurator/guides/micro-controller-comparison.html){: width="250" }](https://sanjay900.github.io/guitar-configurator/guides/micro-controller-comparison.html) for a list of pros and cons about each microcontroller.
```
```danger
Be careful that you don't ever provide 5v power to the power pin of a Wii Extension, as they are not designed for this. The data pins however are tolerant of 5v, so you can hook these up directly to pins on your microcontroller.
```

## The finished product
[![Finished adaptor](../assets/images/adaptor.jpg){: width="250" }](../assets/images/adaptor.jpg)

## Steps
1. Connect wires between the SDA and SCL pins on your breakout board / wii extension cable. 
Refer to the following image for the pinout of a Wii Extension connector. For a Pi Pico, you can pick pins but only specific ones work, so open the configuration tool and choose pins that way.

    [![Finished adaptor](../assets/images/wii-ext.jpg){: width="250" }](../assets/images/wii-ext.jpg)
   
   | Microcontroller | SDA | SCL |
   | --- | --- | --- |
   | Pro Micro, Leonardo, Micro | 2 | 3 |
   | Uno, Pro Mini | A4 | A5 |
   | Mega | 20 | 21 |

```danger
If you are using a wii extension cable do NOT rely on the colours, the manufacturers are all over the place with this and the only way to validate them is to test each wire according to the above image. I've come across connectors wired with green as ground and black as VCC before, you just can't rely on the colours at all unfortunately.
```
1. Connect the vcc on the microcontroller to the vcc on the breakout
   * If you are using a 5v pro micro, and your breakout does not support 5v input, then you will need to hook up VCC from the microcontroller to a 3.3v regulator, and then hook up the output of the regulator to the breakout
   * If you are using the microcontroller uno, use the 3.3v pin on your microcontroller as VCC
   * If you are using the breakout linked ![above](https://www.adafruit.com/product/4836), the `vin` pin is used for both 3.3v input and 5v input. The 3v pin is actually an output and is not needed for this project.
2. Connect the gnd pin on the wii breakout / extension cable to the gnd on your microcontroller.

Now that you have wired your adapter, go [configure it](https://santroller.tangentmc.net/tool/using.html).
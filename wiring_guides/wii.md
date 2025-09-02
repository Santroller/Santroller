---
sort: 5
---

# Setting up an Adaptor for a Wii Controller

## Supplies

- A microcontroller

  - The Pi Pico 1 and Pi Pico 2, or microcontrollers based on these, are currently the only supported microcontrollers.

    {% include sections/microcontrollers.md %}

- A Wii extension breakout board or an extension cable, such as [![this](https://www.adafruit.com/product/4836)](https://www.adafruit.com/product/4836). You can also choose to cut the end of the extension and solder your own cables on as well if you perfer.
- If your wii extension breakout does not support 3.3v input, and you are using a 5v Pro Micro, you will need a 3.3v voltage regulator. The breakout listed above does however support either voltage so this is not required for that breakout.
- Some Wire
- A Soldering Iron

- Supplies for specific features
  - Tilt
    - Any of the following options are supported
      - A Digital Tilt Switch (somtimes called a mercury or ball tilt switch)
        - I recommend using two tilt sensors in series, as this can help with accidental activations
      - The ADXL345, MPU-6050 or LIS3DH Accelerometer
        - This gives a proper analog value of the devices acceleration
          - Tilt is detected by observing acceleration due to gravity
      - An analog accelerometer, such as the ADXL335, or the accelerometer on some GH guitars
        - This gives a proper analog value of the devices acceleration
          - Tilt is detected by observing acceleration due to gravity

```danger
Be careful that you don't ever provide 5v power to the power pin of a Wii Extension, as they are not designed for this. The data pins however are tolerant of 5v, so you can hook these up directly to pins on your microcontroller.
```

## The finished product

[![Finished adaptor](/assets/images/adaptor.jpg)](/assets/images/adaptor.jpg)

## Steps

1.  Connect wires between the SDA and SCL pins on your breakout board / wii extension cable.
    Refer to the following image for the pinout of a Wii Extension connector.

    The Pi Pico lets you pick from various pins for the SDA and SCL pins. We provide recommended pins below, and this pinout is the same as the old Ardwiino firmware. If you need to use other pins, the options are provided below but the SDA and SCL pins must be from the same channel.

    [![pinout](/assets/images/wii.png)](/assets/images/wii.png) [![Finished adaptor](/assets/images/wii-ext.jpg)](/assets/images/wii-ext.jpg)

    ```danger
    If you are using a wii extension cable do NOT rely on the colours, the manufacturers are all over the place with this and the only way to validate them is to test each wire according to the above image. I've come across connectors wired with green as ground and black as 3.3V before, you just can't rely on the colours at all unfortunately.
    ```

    | Microcontroller               | SDA                              | SCL                              |
    | ----------------------------- | -------------------------------- | -------------------------------- |
    | Pi Pico (Recommended)         | GP18                             | GP19                             |
    | Pro Micro, Leonardo, Micro    | 2                                | 3                                |
    | Uno                           | A4                               | A5                               |
    | Mega                          | 20                               | 21                               |
    | Pi Pico (Advanced, Channel 0) | GP0, GP4, GP8, GP12, GP16, GP20  | GP1, GP5, GP9, GP13, GP17, GP21  |
    | Pi Pico (Advanced, Channel 1) | GP2, GP6, GP10, GP14, GP18, GP26 | GP3, GP7, GP11, GP15, GP19, GP27 |

2.  Connect the V<sub>CC</sub> on the microcontroller to the V<sub>CC</sub> on the breakout
    - If you are using a 5v Pro Micro, and your breakout does not support 5v input, then you will need to hook up V<sub>CC</sub> from the microcontroller to a 3.3v regulator, and then hook up the output of the regulator to the breakout
    - If you are using the microcontroller uno, use the 3.3v pin on your microcontroller as V<sub>CC</sub>
    - If you are using the breakout linked ![above](https://www.adafruit.com/product/4836), the `vin` pin is used for both 3.3v input and 5v input. The 3v pin is actually an output and is not needed for this project.
3.  Connect the gnd pin on the wii breakout / extension cable to the gnd on your microcontroller.
4.  If you would like to use tilt:
    <details>
      <summary>Digital Tilt</summary>

    1. Connect one pin of the first tilt sensor to ground
    2. Connect the other pin of the first tilt sensor to a pin on the second tilt sensor
    3. Connect the other pin of the second tilt sensor to a digital pin on your microcontroller.
    4. Affix the sensors to your guitar. You will need to play around with their position to get them to activate at the exact point you want tilt activating.
    </details>

    <details>
      <summary>Analog Tilt</summary>

    1. Connect GND to GND
    2. Connect V<sub>CC</sub> to V<sub>CC</sub>
    3. Connect the signal pin to an analog pin on your microcontroller
    </details>

    <details>
      <summary>ADXL345 Or MPU 6050 or LIS3DH</summary>

    1. Connect GND to GND
    2. Connect V<sub>CC</sub> to V<sub>CC</sub> (note that this is a 3.3v device, so for 5V microcontrollers make sure your breakout has a voltage regulator onboard or you are using your own regulator)
    3. Hook up SDA and SCL to the microcontroller

      | Microcontroller               | SDA                              | SCL                              |
      | ----------------------------- | -------------------------------- | -------------------------------- |
      | Pi Pico (Recommended)         | GP18                             | GP19                             |
      | Pro Micro, Leonardo, Micro    | 2                                | 3                                |
      | Uno                           | A4                               | A5                               |
      | Mega                          | 20                               | 21                               |
      | Pi Pico (Advanced, Channel 0) | GP0, GP4, GP8, GP12, GP16, GP20  | GP1, GP5, GP9, GP13, GP17, GP21  |
      | Pi Pico (Advanced, Channel 1) | GP2, GP6, GP10, GP14, GP18, GP26 | GP3, GP7, GP11, GP15, GP19, GP27 |
    </details>

## Programming

1.  Start Santroller with your microcontroller plugged in.
2.  Set the `Connection Method` to `Wii Adapter`.
3.  Set `Device to Emulate` for the type of device you want to emulate. 
4.  For guitars, if you wish to add tilt, 
5.  Hit Configure
6.  If you are on the Pi Pico and are using different pins, set your SCL and SDA pin.
7.  Hit `Save Settings`.
8.  `Windows controller mode` can be set based on your preferences. Note that this only affects windows, a controller in XInput mode will use the correct mode on a console, and will automatically use HID mode on Linux and macOS.
    1. `XInput` - This works more natively on windows, and most games will automatically bind controls.
    2. `HID` - This uses HID on windows, which means games won't automatically bind controls, but HID is polled a bit more efficiently in games like Clone Hero.
9.  If you would like to adjust settings related to polling, click on `Controller Poll Settings`
10. If you would like to use `Queue Based Inputs` you can turn that on here. This puts any buttons you press into a queue, and sends them out to the PC as fast as USB allows. This means that the controller will process your inputs at a faster poll rate than 1ms (the fastest rate USB allows) and then the PC will be sent your inputs at a 1ms rate.
11. Set the Poll Rate to your preferred setting. 0 sends data as fast as possible, any other number polls inputs at that speed.
    1.  Note that for `Dj Hero Turntables` this needs to be set to 10, otherwise the platter won't work correctly.
12. Debounce can be adjusted here. Debounce is necessary as button inputs are noisy. When you hit a button, it will often bounce and send multiple presses, which some games may percieve as you hitting the button multiple times, which can result in dropped sustains. When you set debounce to a value, the signal ignores any release inputs for that time frame, so if you for example set it to 1ms, then the button input will be on for a minimum of 1ms, and only after that will the release be processed. This has the effect of stretching out the button press to at least 1ms, which ignores any bouncing in that timeframe.
13. Combined strum debounce shares the debounce timeframe between both strums (only available on guitars). This means that if you set the strum debounce to 1ms and strummed down, both strum up and down inputs are ignored over that 1ms timeframe. This helps avoid extra strum inputs when your strum switch rebounds after being released.
14. Click on `Wii Extension Inputs`
15. If you are using a Pi Pico, specify the SDA and SCL pins that you wired your extension port to.
16. Hit Save.
17. You can now calibrate your inputs. If your controller is already plugged in it should be detected and show only those inputs, but you can use the `Wii Inputs to display below` dropdown to specify the controller you want to configure.
    1.  Joysticks or crossfader
        1.  Hit the `Calibrate` button and then move the stick all the way left / up
        2.  Hit `Next` and then move the stick all the way right / down
        3.  Hit `Next` and then move stick a little bit away from the center. This sets the deadzone, and any inputs closer to the center from this position will be ignored, which helps with noisy sticks.
    2.  Whammy
        1.  Click on `Calibrate`.
        2.  Release the whammy bar and hit `Next`.
        3.  Push the whammy all the way in, and hit `Next`
        4.  Release the whammy again, and hit `Next`. If your whammy is noisy, you can push it in a tiny bit, and the zero position will be set to this location, which will make sure that the whammy is always considered released when it is released.
    3.  Turntable Spin
        1.  If your turntable isn't responsive enough in games, you can set a multiplier, which will multiply the value coming from the table by a constant amount.
18. If you would like to use Tilt, make sure you are emulating a guitar, and then hit `Add Setting` and add a `Tilt` setting. Then follow one of the below options:
    <details>
      <summary>Digital Tilt</summary>

    1. Click on Tilt, and make sure the `Input Type` is set to `Digital Pin Input`.
    1. Make sure `Pin Mode` is set to `Pull Up`.
    2. Click on the `Find Pin` button, and then tilt your guitar. If you have wired everything correctly, the tool should detect the pin and the tilt icon should light up whenever you tilt the guitar.
    3. If you are using a SW520D based tilt sensor, some versions of this sensor will have an inverted output. You can turn on the `Invert` option to correct this.
    </details>

    <details>
      <summary>Analog Tilt</summary>

    4. Click on Tilt, and make sure the `Input Type` is set to `Analog Pin Input`.
    5. Click on the `Find Pin` button, and tthen tilt your guitar. If you have wired everything correctly, the tool should detect the pin and the tilt `Original Value` value should change as you tilt your guitar.
    6. Click on `Calibrate`
    7. Tilt your guitar down, and then hit `Next`
    8. Tilt the guitar up, and then hit `Next`
    9. Hold your guitar in its resting position and then hit `Next`.
    </details>

    <details>
      <summary>ADXL345 or MPU-6050 or LIS3DH based Tilt</summary>

      1. Click on `Add setting`
      2. Add an `Accelerometer`
      3. For the Pi Pico, set the SDA and SCL pins that you have used.
      4. Hit save.
      5. Click on Tilt, and make sure the `Input Type` is set to `Accelerometer Input`.
      6. Click on `Calibrate`
      7. Hold your guitar in its resting position, and then hit `Next`
      8. Tilt the guitar up, and then hit `Next`
      9. Tilt your guitar up a little bit and then hit `Next`. Values below this position will be zeroed. With the adxl, you can increase the deadzone to help counteract strumming or shaking activating tilt.
      10. Adjust the `Low Pass Filter`. This value controls how new value from the ADXL are filtered, a value closer to 0 will result in a sensor that won't respond to a shake or strumming, but if you decrease it too much the sensor will have a decreased responsiveness. If you set it closer to 1, then the sensor will be very responsive but it will also pick up any tiny vibrations as well. From testing a value of 0.05 seemed like a good place to start. 
    </details>

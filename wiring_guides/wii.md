---
sort: 5
---

# Setting up an Adapter for a Wii Controller

This guide walks through the hardware and software setup required to use a Wii controller extension with Santroller.

---

## Supplies

### Required

- **Microcontroller**

  - Pi Pico 1, Pi Pico 2, or microcontrollers based on them are currently the only supported options.
  - Previous versions of Santroller supported additional microcontrollers; information about those can be found below.

  {% include sections/microcontrollers.md %}

- **Wii Extension breakout board or extension cable**

  - Example: [Adafruit Wii Nunchuck Breakout Adapter](https://www.adafruit.com/product/4836)
  - You may also cut the end off an extension cable and solder directly if preferred

- **Wire**

- **Soldering iron**

### Power considerations

- If your Wii Extension breakout does **not** support 3.3 V input and you are using a 5 V microcontroller, you will need a 3.3 V voltage regulator
- The Adafruit breakout linked above supports both 3.3 V and 5 V input and does not require an external regulator

### Optional features

#### Tilt support

Any of the following options are supported:

- **Digital tilt switch** (sometimes called a mercury or ball tilt switch)

  - Using two tilt switches in series is recommended to reduce accidental activations

- **Digital accelerometers**: ADXL345, MPU-6050, or LIS3DH

  - Provides analog acceleration data
  - Tilt is detected using acceleration due to gravity

- **Analog accelerometers**: ADXL335 or accelerometers found in some Guitar Hero guitars

  - Provides analog acceleration data
  - Tilt is detected using acceleration due to gravity

```danger
Never supply 5 V power to the power pin of a Wii Extension. Wii Extensions are not designed for 5 V input.

The data pins *are* 5 V tolerant and may be connected directly to microcontroller GPIO pins.
```

---

## Finished product

![Finished Adapter](https://santroller.com/assets/images/adaptor.jpg)

---

## Wiring steps

### 1. Connect SDA and SCL

Connect the SDA and SCL pins from your Wii Extension breakout or extension cable to your microcontroller.

Refer to the image below for the Wii Extension connector pinout.

The Pi Pico allows multiple pin choices for SDA and SCL. Recommended pins are listed below and match the legacy Ardwiino firmware layout. If using different pins, both SDA and SCL **must be from the same I²C channel**.

![Port Pinout](https://santroller.com/assets/images/wii-ext.jpg)
![Port Schematic](https://santroller.com/assets/images/wii.png)
&#x20;

```danger
If you are using a Wii Extension cable, do NOT rely on wire colors.

Manufacturers use inconsistent coloring. Always verify each wire using the pinout above. Some cables have been observed with green as ground and black as 3.3 V.
```

| Microcontroller               | SDA                              | SCL                              |
| ----------------------------- | -------------------------------- | -------------------------------- |
| Pi Pico (Recommended)         | GP18                             | GP19                             |
| Pro Micro / Leonardo / Micro  | 2                                | 3                                |
| Uno                           | A4                               | A5                               |
| Mega                          | 20                               | 21                               |
| Pi Pico (Advanced, Channel 0) | GP0, GP4, GP8, GP12, GP16, GP20  | GP1, GP5, GP9, GP13, GP17, GP21  |
| Pi Pico (Advanced, Channel 1) | GP2, GP6, GP10, GP14, GP18, GP26 | GP3, GP7, GP11, GP15, GP19, GP27 |

---

### 2. Connect power (VCC)

- Connect the microcontroller VCC to the breakout VCC
- If using a 5 V Pro Micro and a breakout that does not support 5 V input, route VCC through a 3.3 V regulator
- On the Arduino Uno, use the onboard 3.3 V pin
- On the Adafruit breakout:
  - `VIN` accepts both 3.3 V and 5 V input
  - The `3V` pin is an output and is not required

---

### 3. Connect ground

Connect GND on the Wii Extension breakout or cable to GND on the microcontroller.

---

### 4. Optional: tilt wiring

#### Digital tilt

1. Connect one pin of the first tilt switch to ground
2. Connect the other pin of the first switch to one pin of the second switch
3. Connect the remaining pin of the second switch to a digital input on the microcontroller
4. Mount the switches on the guitar and adjust placement to control activation angle

#### Analog tilt

1. Connect GND to GND
2. Connect VCC to VCC
3. Connect the signal pin to an analog input

#### ADXL345 / MPU-6050 / LIS3DH

1. Connect GND to GND
2. Connect VCC to 3.3 V (use a regulator if required)
3. Connect SDA and SCL using the table above

---

## Programming

1. Launch Santroller with the microcontroller connected
2. Set **Connection Method** to `Wii Adapter`
3. Set **Device to Emulate** to the desired controller type
4. Click **Configure**
5. If using a Pi Pico with custom pins, set the SDA and SCL pins
6. Click **Save Settings**

---

### Windows controller mode

This setting only affects Windows behavior. Consoles and other operating systems automatically select the correct mode.

- **XInput**

  - Native Windows support
  - Most games auto-bind controls

- **HID**

  - No automatic binding
  - Slightly more efficient polling in games like Clone Hero

---

### Polling and debounce

- **Controller Poll Settings** allows adjustment of polling behavior
- **Queue Based Inputs** queues inputs internally and sends them at the USB maximum rate (1 ms)

Poll rate settings:

- `0` sends data as fast as possible
- Any other value polls at the specified rate
- **DJ Hero turntables must use a poll rate of 10**

Debounce:

- Filters noisy button signals
- Prevents dropped sustains caused by button bounce

Combined strum debounce (guitars only):

- Shares debounce timing between strum up and strum down
- Prevents rebound-triggered extra strums

---

### Wii Extension inputs

1. Open **Wii Extension Inputs**
2. If using a Pi Pico, specify the SDA and SCL pins
3. Click **Save**

---

### Calibration

Use the **Wii Inputs to display below** dropdown if multiple devices are connected.

#### Joysticks / crossfader

1. Click **Calibrate** and move fully left or up
2. Click **Next** and move fully right or down
3. Click **Next** and move slightly off center to set the deadzone

#### Whammy bar

1. Click **Calibrate**
2. Release the whammy and click **Next**
3. Fully press the whammy and click **Next**
4. Release again and click **Next**

#### Turntable spin

- Adjust the multiplier if responsiveness is too low

---

### Configuring tilt in Santroller

Ensure you are emulating a guitar, then click **Add Setting** and add **Tilt**.

#### Digital tilt

1. Set **Input Type** to `Digital Pin Input`
2. Set **Pin Mode** to `Pull Up`
3. Click **Find Pin** and tilt the guitar
4. Enable **Invert** if using an inverted-output sensor such as some SW520D variants

#### Analog tilt

1. Set **Input Type** to `Analog Pin Input`
2. Click **Find Pin** and tilt the guitar
3. Click **Calibrate**
4. Tilt down and click **Next**
5. Tilt up and click **Next**
6. Hold the resting position and click **Next**

#### Accelerometer-based tilt

1. Click **Add Setting** and add **Accelerometer**
2. Set SDA and SCL pins if using a Pi Pico
3. Click **Save**
4. Set **Input Type** to `Accelerometer Input`
5. Click **Calibrate**
6. Hold the resting position and click **Next**
7. Tilt up and click **Next**
8. Tilt slightly upward and click **Next** to set the deadzone
9. Adjust the **Low Pass Filter**
   - Values near 0 reduce shake sensitivity but lower responsiveness
   - Values near 1 increase responsiveness but amplify noise
   - A starting value of 0.05 is recommended


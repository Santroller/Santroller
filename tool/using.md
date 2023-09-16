---
sort: 1
---
# Using the Santroller Configurator
The Santroller Configurator is a tool that can program any micro controller with the Santroller firmware.

## How to use the Configurator
1. Open the Configurator. If this is your first time using it, it will need to extract some libraries and this may take a little bit.
2. Plug in your microcontroller.
3. Choose an `Input Type` to load the firmware with a sensible default configuration.
    - Directly Wired
        - Choose this if you are following the [guitar](https://santroller.tangentmc.net/wiring_guides/guitar.html), [drum](https://santroller.tangentmc.net/wiring_guides/drum.html) or [turntable](https://santroller.tangentmc.net/wiring_guides/turntable.html) guides.
    - Wii Adapter
        - Choose this if you are following the [Wii adapter](https://santroller.tangentmc.net/wiring_guides/wii.html) guide.
    - PS2 Adapter
        - Choose this if you are following the [PS2 adapter](https://santroller.tangentmc.net/wiring_guides/ps2.html) guide.
    - USB Adapter
        - Choose this if you are following the [USB adapter](https://santroller.tangentmc.net/wiring_guides/usb.html) guide, and just want to use this for USB inputs.
    - Bluetooth Receiver
        - Choose this if you are following the [Bluetooth receiver](https://santroller.tangentmc.net/tool/bluetooth.html) guide.
    - Peripheral Device
        - Choose this if you are following the [Peripheral](https://santroller.tangentmc.net/tool/peripheral.html) guide.
        - You will need to set your SCL and SDA pins on the peripheral that are connected to the main controller. This is the pins being used on the peripheral end, not the main controller end.
4. Hit `Configure`
5. Wait for the initial configuration, and then hit `Configure` again.
6. You will now be at the main screen of the tool. From here, you can configure every option of your controller.

  [![Main Screen](../assets/images/screenshots/main.png){: width="250" }](../assets/images/screenshots/main.png)
7. There are two main areas of this screen, the sidebar and the options list. In the sidebar you have several buttons:
    - Mode
        - You can change the mode between `Simple` and `Advanced`, which allows for configuring some options that most people won't need, such as setting individual debounce for each input, and configuring if a pin is a `pull-up`, `pull-down` or is `floating`.
    - Controller View Type
        - This will change the legends and icons for various buttons, to allow for seeing how things are bound in the context of different controllers. For example, the `Guide` button will be the `Guide` button in Xbox mode, the `PS` button in Playstation mode, and the `Home` button in switch mode.
    - Add Setting
        - This button is the heart of the program, and it allows for adding new options to the options list.
    - Save Settings
        - If your configuration is valid, this will allow you to save your configuration to the connected microcontroller.
    - Reset Settings
        - When this button is clicked, it will give you the ability to either clear all options from the options list, or reset the options to the default list.
    - Import / Export Settings
        - This will allow for saving the configuration from your controller to a file, and then allow you to load it back.
    - Configure all inputs
        - This will give you an interface that will go through each input in the options list, and allow you to press a button or move an axis to bind. This can be a faster way to bind multiple inputs at once.
    - Revert device to Arduino
        - This will revert the microcontroller back to a form that is recognised by the Arduino IDE.
    - Back to main menu
        - This will take you back to the main menu, and allow you to pick another device to program.
8. The options list gives you the ability to change pretty much any setting you should ever want to change. Click on an option to expand it and see its individual settings.
9. Click on `Controller Settings`. From here, you can change the `Emulation Type`, which allows for emulating controllers, keyboards, bluetooth controllers or bluetooth keyboards.
10. You can set your `Controller Type` from here as well. This is what tells the firmware what type of device you want to emulate, and from here you can pick things like if you are emulating a guitar or a drum kit.
11. `Swap Switch Face Buttons` can be used to swap how the face buttons for switch emulation are bound. This is necessary, as you can build USB adapters, and when mapping inputs from a Xbox controller to a Switch controller, some people will want the face buttons to keep the same names, while others will want the face buttons to keep the same position.
12. `Windows Controller Mode` can be used to swap between emulating XInput or a standard HID controller on Windows. Note that for any other platform, the relevant mode will be used automatically. so an Xbox 360 will use XInput, but a macOS or Linux PC will use HID.
13. Expand `Controller Poll Settings`
14. From here, you can change your debounce settings, and your poll rate. You can also enable `Queue based inputs` here too. This setting will poll your inputs as fast as possible, and then it will put them in a queue and send them to your device at the dictated rate. This means that the console or PC you are connected too no longer has to poll your device at max speed, as you can now enter inputs and they will be queued until the next queue period.
15. If you have followed the [RGB LED](https://santroller.tangentmc.net/wiring_guides/led.html) guide, then set your LED Type to APA102 here. Note that different manufactuers have their APA102s work with colours in different orders, so if you find that colours you pick in the tool are totally different to what you see on your LEDs, then you have probably picked the wrong colour order.
17. You can now click on different inputs to configure them.
    - Digital Inputs
        - For digital inputs, you can pick the input type, in order to map an input from any supported device to this output.
        - You can then also pick if you want this input inverted before it gets mapped to the output.
        - The icon for the input will light up when the input is activated, which gives you an easy way to test if your input is working.
        - If you map an analog input here, then you can dictate a threshold that needs to be reached before an input is considered activated.
        - If you have APA102 LEDs enabled, you will be able to pick the colour used when the input is triggered, or when it is not triggered. You can also select which leds in the chain will be updated by this input.
    - Analog Inputs
        - For analog inputs, you can pick the input type, in order to map an input from any supported device to this output.
        - You will then be able to see the raw and calibrated values from your analog input live.
        - You can then hit the `Calibrate` button to calibrate your input. Follow the instructions and this will allow you to calibrate the raw values.
        - You can also manually change the sliders to fine tune your calibration manually.
        - If you map a digital input here, you can instead just specify what value that digtal input will represent when triggered,
        - If you have APA102 LEDs enabled, you will be able to pick a active and inactive colour. The controller will smoothly transition from inactive to active depending on the state of the analog input. You can also select which leds in the chain will be updated by this input.
    - Macro Inputs
        - For digital inputs, its possible to specify a macro input. This will require you to press multiple buttons at once to trigger an output, for example, this allows for things like pressing start + select to emulate home on a wii guitar.
    - Multiplexer Inputs
        - This allows for using more analog inputs than your micro controller supports.
        - You can select your Analog Input, S0, S1 and S2 Pins. For a 16 channel multiplexer, you can also set S3.
        - You can then pick the channel that you want to use as an input.
    - Peripheral Settings
        - This allows for configuring the peripheral controller. This only works with the Pi Pico, and allows for using a second Pi Pico to combine multiple inputs together, for example to allow keeping guitar necks removable if the neck does not provide enough pins, or to allow putting LEDs in a DJ hero turntable.
        - Once this option is switched on, you will see peripheral versions of various inputs in the input type list for digital and analog inputs.
        - Set the SCL and SDA pins you have connected to the peripheral controller. Here you pick the pins being used on the main Pico, not the peripheral one.
    - Wii Extension Inputs
        - This allows for configuring Wii Extension Inputs. Note that if this setting has just been added, you won't see any data coming from it until you hit `Save Settings`.
        - For the Pi Pico, you can pick your SDA and SCL pins here.
        - The `Wii Inputs to display below` dropdown allows for filtering out which inputs will show in this list. This allows for making it easier to find an input you may want to configure. This does not change what inputs are programmed and usable, only what is currently displayed.
        - You can disable an input here if you don't want it to function. This can be used to disable buttons that you do not want to use, for example if you have disconnected the joystick on a wiitar, you can disable it here so that it doesn't send phantom inputs.
        - For analog inputs, the same calibration steps outlined above in the `Analog Inputs` section also apply.
        - Note that if you want a bespoke config, you can also disable an input here, and instead add an output manually using the `Add Setting` button. This can allow you to do things like mapping the `A` button on a wii controller to a joystick input, or whatever else you can think of, and then you in that case would probably want to disable the normal binding.
    - PS2 Controller Inputs
        - This allows for configuring PS2 Controller Inputs. Note that if this setting has just been added, you won't see any data coming from it until you hit `Save Settings`.
        - For the Pi Pico, you can select your Command, Data, Clock, Acknowledge and Attention pins here.
        - The `PS2 Controller Inputs to display below` dropdown allows for filtering out which inputs will show in this list. This allows for making it easier to find an input you may want to configure. This does not change what inputs are programmed and usable, only what is currently displayed.
        - You can disable an input here if you don't want it to function. This can be used to disable buttons that you do not want to use, for example if your PS2 guitars tilt is broken, you could disable that here to stop it from sending phantom inputs.
        - For analog inputs, the same calibration steps outlined above in the `Analog Inputs` section also apply.
        - Note that if you want a bespoke config, you can also disable an input here, and instead add an output manually using the `Add Setting` button. This can allow you to do things like mapping the `X` button on a PS2 controller to a joystick input, or whatever else you can think of, and then you in that case would probably want to disable the normal binding.
    - USB Host Inputs (Pi Pico only)
        - This allows for configuring USB device Inputs. Note that if this setting has just been added, you won't see any data coming from it until you hit `Save Settings`.
        - You can pick your D+ pin here. Due to how the implementation works, the D- pin is automatically set to the next pin, as the pins need to be next to eachother.
        - You can also see any devices that have been detected by the usb port. Note that only a few rhythm game controllers, first party gamepads and xbox 360 controllers are supported at the moment.
        - You can disable an input here if you don't want it to function. This can be used to disable buttons that you do not want to use, for example if your gamepad has an input you don't want you can disable it here.
        - For analog inputs, the same calibration steps outlined above in the `Analog Inputs` section also apply.
        - Note that if you want a bespoke config, you can also disable an input here, and instead add an output manually using the `Add Setting` button. This can allow you to do things like mapping a button from a usb gamepad to a fret to build custom pad configs.
    - GH5 Neck Inputs
        - This allows for configuring GH5 Neck Inputs. Note that if this setting has just been added, you won't see any data coming from it until you hit `Save Settings`.
        - For the Pi Pico, you can select your SCL and SDA pins here.
        - You can also pick if you want the Slider to map to standard frets, or if you want to disable the normal frets because you have hardwired them. You can also disable the slider outright here as well.
    - GHWT Neck Inputs (Pi Pico only)
        - This allows for configuring GHWT Neck Inputs. Note that if this setting has just been added, you won't see any data coming from it until you hit `Save Settings`.
        - You can select your Analog Input, S0, S1 and S2 Pins.
        - The slider bar works based on capacatance, so the sensitivity allows for tuning how sensitive you want your frets to be. If you make this too high a value, then the fret will flicker as it won't always detect your finger, but if you make it too low a value, then the frets will mistrigger as they will pick up random capacatance fluctuations, or pressing nearby frets will activate inputs due to the capacatance changes.
        - You can also pick if you want the Slider to map to standard frets, or if you want to disable the normal frets because you have hardwired them. You can also disable the slider outright here as well.
    - Crazy Guitar Neck Inputs
        - The Crazy Guitar is a 3rd party guitar hero guitar, which has a similar neck pinout to the GH5 necks.
        - For the Pi Pico, you can select your SCL and SDA pins here.
        - You can also pick if you want the Slider to map to standard frets, or if you want to disable the normal frets because you have hardwired them. You can also disable the slider outright here as well.
    - LED Binding
        - This allows you to pick an event (such as a player led) and bind a digital output or an APA102 LED to it.
    - Rumble Binding
        - This is only really relevant for emulating actual gamepads.
        - This allows you to take the rumble data being sent by the console and send it to a motor driver.
        - Pick the motor drivers pin here.
    - Console Mode Binding
        - Normally the controller will detect the console its plugged into, and go into the right mode for that console.
        - However, you may want to force a specific mode, for example you may want to use a console mode on PC for an emulator.
        - This will allow you to set an input, and then if that input is held on startup, the controller will jump to the mode you specify.
    - Controller Reset
        - When you specify an input here, the controller will reset itself. This can help if something goes wrong with authentication.
    - Bluetooth Inputs
        - Follow the [bluetooth](https://santroller.tangentmc.net/tool/bluetooth.html) guide for more information about this setting.
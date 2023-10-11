---
sort: 5
---
# Commercial use of the Santroller platform
The standard tool does not license you to use it for profit. 
However, there is a commercial version of it that can be obtained by subscribing to the projects [GitHub Sponsers](https://github.com/sponsors/sanjay900).
{% include sponser.html %}

# Features of the commercial tool
* Only the RP2040 is supported by this tool, as we leverage its features to simplify programming for your customers.
* Custom branding, including customisation of device names, logos and colours.
* Ability to build a customer facing tool, that hides most configuration options besides the basics that a customer would need.
  * This will hide things like pin configuration, so that an end user won't be able to mess up their configuration, to avoid support requests after the product is sold.
* Ability to export a UF2 file for easy automation of programming multiple devices at once.
* Simplified tool for the end user, that takes up much less space and programs their devices much quicker.
  * The tool also allows for easy factory resetting as well, which can give you an easy method to revert a customers configuration when providing support.
* Paying for this tool also goes towards providing continual improvements to the tool.  

# Using the commercial tool
When you are subscribed to the tool, you will gain access to the [commercial tool releases](https://github.com/Santroller/SantrollerConfiguratorBinaries/releases)
From here, you can download the `SantrollerConfiguratorBuilder` for your operating system.
Once you launch this tool, you will be presented with a screen for configuring a custom tool with your branding and devices.

## Creating a tool
This tool allows for managing multiple different branded tools. Click on `Create Tool` to create a tool.
Enter a name for your tool in the `Tool Name` text box to name your tool. This name will be used for the executable name, and for the window title.

## Colours
The `Primary Colour`, `Warning Colour` and `Error Colour` change the colours used by the progress bar, and by buttons in the primary state. This can be used to theme the app to a custom colour scheme.

## Logo
The Logo is displayed when the app is opened. The Logo needs to be a PNG.

## Icon
The Icon is used for the application icon. The icon needs to be a PNG and must be square.

## Variants
A variant describes a configuration for a device. For example, if you are selling multiple adapters, you would create a variant for each adapter. For Wii adapters, you may even want to create different variants for different configurations, such as one for Guitars, one for Gamepads and one for Drums.
Hit the `Create Variant` button to create a device variant. The `Variant Name` is the name the actual device will use when it is plugged into a computer. the `Variant Device Vendor` is the vendor name that is used when plugged into a computer.
Hit `Configure Variant` to set a configuration. This will bring you to a screen that acts very similar to the standard configuration tool. You can follow the instructions for the [standard tool](https://santroller.tangentmc.net/tool/using.html#the-main-screen) for more information about how to configure a device here. If you want to configure things like Wii adapters, you will need to remove the standard inputs and then use the `Add Setting` button to add it. The `Reset Settings` button allows for removing all inputs at once. 

You can use the `Write Configuration to device` button to test your configuration on a device. Pick the device you want to test from the list, and its configuration will be overwritten with the configuration you are building.
You can use the `Export UF2` button to save a `UF2` file with your configuration. This can be directly written to a Pi Pico, to allow for easy bulk programming.
# Console Detection
Console detection happens in multiple passes, depending on the console in question

# PS3 and PS4
We include the required usages below on the same report, as for some reason the PS3 will only request the feature if we do that.
This means we need to do a seperate step to differenciate PS3 and PS4. The easiest way to do this, is to jump to PS4 mode, and then jump to PS3 if we dont see it requesting the feature report again.
# PS3
If you include a feature report in your hid report descriptor, with usage 0x2621, then the ps3 will ask for this and you can catch that and jump to a PS3 compat mode.
Interestingly, for controllers the PS3 requires a bit of extra auth BUT it doesn't actually require this for a standard hid device, and the PS3 works just fine with a normal hid device too, so for gamepad mode we actually jump to a seperate PS3 specific PS3 compat mode, that neither PADEMU or fakemote use.

# Wii (fakemote)
Since i wrote the implementations for both of these, i know they send out feature report for 0xF0 and 0xF2, and we catch that and jump to a ps3 compat mode.

# Wii (RockBand) + Wii U (GHL)
The wii and wii u both just stop talking to the device if they don't recognise it, but they do this after configuring it. This means we can just run a timer, and if nothing attempts to read the hid report descriptor within 2 seconds we can assume we are on a wii or a wii u.

# PADEMU
Similar to the Wii / Wii U above but the device never actually gets configured, only its descriptors are read. Same method works except we can test if the device was configured or not.
# PS4
If you include a feature report of 0x03 in your hid report, with a usage of 0x2721, then the PS4 will ask for this and we jump to a ps4 compat mode. Note that for Guitars and Drums, this actually falls back to the PS3 versions.

# xb360
If you include a interface that looks like the xbox 360 security interface, the xb360 will try to do auth, and this can be caught and we then jump to a xb360 mode

# Windows
With windows, we catch the WCID report query and jump to a windows + xbone mode. We set the WCID query for XB360 mode, and then windows will send us a hid interrupt request to set up controller LEDs, and we catch this and jump to xb360 mode

# Xbox one
We do a similar thing to the windows method, but during that detection state, we also send out a xbone Announce packet. The xbone responds to this, and we pick that up and jump to a proper xbone compat mode. The xbone does not actually appear to check WCID which seems reasonable since it doesn't need to support anything besides xbone controllers.

# Switch
The switch clears both the in and out endpoint, and we detect this to jump to a switch compat mode

# Mac / linux / android
We stay in a "Universal" mode if none of the above stuff is detected, which will just use standard HID
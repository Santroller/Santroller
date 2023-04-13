# Console Detection
Console detection happens in multiple passes, depending on the console in question

# PS3
PS3 sends out a hid feature request with report id 0x00, and we can catch this and jump to a PS3 compat mode.
Interestingly, for controllers the PS3 requires a bit of extra auth BUT it doesn't actually require this for a standard hid device, and the PS3 works just fine with a normal hid device too, so for gamepad mode we actually jump to a seperate PS3 specific PS3 compat mode, that neither PADEMU or fakemote use.

# Wii (fakemote) + PS2 (PADEMU)
Since i wrote the implementations for both of these, i know they send out feature report for 0xF0 and 0xF2, and we catch that and jump to a ps3 compat mode.

# Wii (RockBand) + Wii U (GHL)
The wii and wii u both just stop talking to the device if they don't recognise it. This means we can just run a timer, and if nothing attempts to read the hid report descriptor within 2 seconds we can assume we are on a wii or a wii u.
# PS4
If you include a feature report of 0x03 in your hid report, then the PS4 will ask for this and we jump to a ps4 compat mode. Note that for Guitars and Drums, this actually falls back to the PS3 versions.

# xb360
If you include a interface that looks like the xbox 360 security interface, the xb360 will try to do auth, and this can be caught and we then jump to a xb360 mode

# Windows
With windows, we catch the WCID report query and jump to a windows + xbone mode. We set the WCID query for XB360 mode, and then windows will send us a hid interrupt request to set up controller LEDs, and we catch this and jump to xb360 mode

# Xbox one
We do a similar thing to the windows method, but during that detection state, we also send out a XBONE Announce packet. The XBOne response to this, and we pick that up and jump to a proper Xbone compat mode.

# Switch
The switch clears both the in and out endpoint, and we detect this to jump to a switch compat mode

# Mac / linux / android
We stay in a "Universal" mode if none of the above stuff is detected, which will just use standard HID
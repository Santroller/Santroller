---
sort: 5
---
# Configuring for Fortnite Festival

For fortnite festival, it makes the most sense to emulate a keyboard, as this avoids the need for auth on consoles, and avoids a bunch of issues on PC. However, we do see that it is useful to be able to configure both standard guitar inputs and keyboard inputs at the same time.
To support this, we have a concept of "Mode Bindings". This means you can hold a button on your guitar when plugging it onto a PC / console, and it will emulate a keyboard.

Note that if your are on the nintendo switch, the default guitar bindings are configured in such a way that you should be able to remap to usable controls within fortnite festival. Keyboard emulation doesn't actually work on the switch, but that doesn't really matter since you don't need auth to use controller mode.

Some people prefer emulating a riffmaster on windows, so we also offer that. However, we can't emulate a real riffmaster due to auth requirements, so we instead emulate enough of a riffmater to trick the game, but as a result this does not have working whammy.

## How to add the mode binding
1. Hit `Add Setting`
3. Pick `Fortnite Festival (Pro Mode - Keyboard)`
   1. This will work on consoles and PC
   2. If you wish to emulate a riffmaster compatible guitar on PC, you can instead use `Fortnite Festival (Pro Mode - PC gamepad)`, but do note that whammy won't work.
   3. You can always add both modes if you wish to be able to use both, just put them on different buttons.
4. Pick the correct `Input Type` and then configure the rest of the settings as necessary. You are configuring a button that when held will jump to keyboard emulation mode.
   1. For example, on wii you may do something like this:

      [![Console Mode Wii](/assets/images/screenshots/fnf_console_wii.png)](/assets/images/screenshots/fnf_console_wii.png)

      This would make it so holding green when pluging in the guitar makes the guitar go into the Fortnite Festival keyboard emulation.
   2. For direct you may do something like this:

      [![Console Mode Direct](/assets/images/screenshots/fnf_console_direct.png)](/assets/images/screenshots/fnf_console_direct.png)

      This would make it so holding the button on pin GP4 when pluging in the guitar makes the guitar go into the Fortnite Festival keyboard emulation.
5. If you wish to play on PlayStation, go into `Controller Settings` and make sure the `Rollover Mode` is set to `6KRO`
6. Save your settings
7. Unplug and replug the guitar, holding the input you just configured. Your guitar should show up as a keyboard now.
8. Play festival
9.  Unplug and replug the guitar without holding a button to go back to normal mode or to configure.
# Xbox 360 DJ Hero Turntable

## Controller Info

- XInput Type: Gamepad (1)
- XInput Subtype: 23, not part of XInput standards
- Vendor ID: `0x1430`
- Product ID: ? (Unsure if reported by wireless turntables)

## Input Info

Face buttons work like a standard Xbox 360 controller.

Tables:

- Left table:

  | Action  | Input                 |
  | :-----  | :---:                 |
  | Scratch | Left stick X          |
  | Green   | A + LT `0b_0000_0001` |
  | Red     | B + LT `0b_0000_0010` |
  | Blue    | X + LT `0b_0000_0100` |

- Right table:

  | Action  | Input                 |
  | :-----  | :---:                 |
  | Scratch | Left stick Y          |
  | Green   | A + RT `0b_0000_0001` |
  | Red     | B + RT `0b_0000_0010` |
  | Blue    | X + RT `0b_0000_0100` |

- Scratching:
  - Positive is clockwise, negative is counter-clockwise.
  - Only uses a tiny range (within -64 to +64), presumably so as to not register on the Xbox 360 menus.

Crossfader: Right stick Y

- Range is inverted: left is positive, right is negative.

Effects knob: Right stick X

- Clockwise increments, counter-clockwise decrements.
- Wraps around when reaching maximum or minimum.

Euphoria button: Y button

## Vibration Info

Euphoria button light: Right vibration

- Via XInput: First turns on at 7936 (`0x1F00`). Maxes out at 65535 (`0xFFFF`).
- Note that in the low-level XUSB report, the vibration values are bytes, not shorts. XInput just divides the motor values by 256 before sending them to the driver, so this means the light first turns on at `0x1F` and maxes out at `0xFF`.
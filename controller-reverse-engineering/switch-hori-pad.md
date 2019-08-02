# Switch Hori Pad
The switch hori pad functions exactly the same as a PS3 controller. Only, the axis data isn't processed at all. This means you can reuse the same code, and just simply change the vid and pid.

## VIDs and PIDs
Note that it is a requirement to use the Hori Pad vid and pid for the game to detect your controller, which is:

| Device   | VID    | PID    |
| -------- | ------ | ------ |
| Hori Pad | 0x0F0D | 0x0092 |

A LUFA implementation of this controller is in [output_ps3.c](/src/shared/output/output_ps3.c)
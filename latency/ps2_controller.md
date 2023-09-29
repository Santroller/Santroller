---
sort: 9
---

# PS2 DualShock 2 Controller

Latency of an input from a PS2 DualShock 2 Controller.

Measured by connecting it to the microcontroller in question, and then sending a signal to toggle cross button input, and timing how long it takes that signal to reach a target device over USB.
Note that the PS2 controllers send a LOT more data over the controller bus, due to the buttons being analog. This in turn increases the latency, and since the buttons are all analog, it also just takes longer for the controller itself to detect an input. Consoles normally don't poll these controllers all that often either.

| Results         |          |          |
| :-------------- | -------- | -------- |
| firmware        | new      | old      |
| microcontroller | Pi Pico  | Pi Pico  |
| poll rate       | 1 ms     | 1 ms     |
| min             | 1.46 ms  | 1.63 ms  |
| max             | 12.24 ms | 65.5 ms  |
| avg             | 6.37 ms  | 15.44 ms |
| stddev          | 2.96 ms  | 11.51 ms |
| %on time        | 62%      | 32%      |
| %1f skip        | 38%      | 55%      |
| %2f skip        | 0%       | 13%      |
| %3f skip        | 0%       | 87%      |

{% include ps2.html %}

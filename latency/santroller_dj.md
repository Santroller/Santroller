---
sort: 11
---

# DJ Hero Turntable

Latency of an input from a DJ Hero Turntable platter. These measurements do not involve the turntable mainboard, the microcontroller is hooked up to the platter directly over I2C.
Note that you can control how quickly you want the turntable polled. However, the turntable sends its velocity data in a relative form as the difference since the last poll, so if you poll the platter too quickly, you end up losing precision on the turntable platter. For this reason, values for both 10ms (the default used by the console) and 1ms are included.

Measured by connecting it to the microcontroller in question, and then sending a signal to toggle the green fret input, and timing how long it takes that signal to reach a target device over USB.

| Results             |          |          |
| :------------------ | -------- | -------- |
| firmware            | new      | new      |
| microcontroller     | Pi Pico  | Pi Pico  |
| turntable poll rate | 10 ms    | 1 ms     |
| poll rate           | 1 ms     | 1 ms     |
| min                 | 21.03 ms | 20.32 ms |
| max                 | 61.29 ms | 30.85 ms |
| avg                 | 40.53 ms | 25.67 ms |
| stddev              | 11.58 ms | 2.57 ms  |
| %on time            | 0%       | 0%       |
| %1f skip            | 11%      | 44%      |
| %2f skip            | 42%      | 56%      |
| %3f skip            | 47%      | 0%       |

{% include turntable.html %}

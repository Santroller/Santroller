---
sort: 1
---

# Direct
Latency of directly hooking an input to a digital pin on a microcontroller

Measured by toggling a pin on the microcontroller in question, and then timing how long it takes for that signal to reach a target over USB.

| Microcontroller | Pi Pico | Sparkfun Pro Micro 5V | Sparkfun Pro Micro 3.3V | Arduino Uno | Pi Pico | Sparkfun Pro Micro 5V | Sparkfun Pro Micro 3.3V | Arduino Uno |
| :-------------- | ------- | --------------------- | ----------------------- | ----------- | ------- | --------------------- | ----------------------- | ----------- |
| Firmware        | new     | new                   | new                     | new         | old     | old                   | old                     | old         |
| Usb Poll Rate   | 1 ms    | 1 ms                  | 1 ms                    | 1 ms        | 1 ms    | 1 ms                  | 1ms                     | 1 ms        |
| Minimum         | 0.53 ms | 0.53 ms               | 0.53 ms                 | 0.48 ms     | 1.32 ms | 0.54 ms               | 0.73 ms                 | 0.92 ms     |
| Maximum         | 1.34 ms | 1.34 ms               | 1.35 ms                 | 1.3 ms      | 2.15 ms | 1.36 ms               | 1.97 ms                 | 1.96 ms     |
| Average         | 0.84 ms | 0.84 ms               | 0.84 ms                 | 0.8 ms      | 1.79 ms | 0.86 ms               | 1.26 ms                 | 1.42 ms     |
| Std dev         | 0.24 ms | 0.24 ms               | 0.24 ms                 | 0.25 ms     | 0.22 ms | 0.25 ms               | 0.31 ms                 | 0.3 ms      |
| %on time        | 96%     | 96%                   | 96%                     | 96%         | 90%     | 96%                   | 93%                     | 92%         |
| %1f skip        | 4%      | 4%                    | 4%                      | 4%          | 10%     | 4%                    | 7%                      | 8%          |
| %2f skip        | 0%      | 0%                    | 0%                      | 0%          | 0%      | 0%                    | 0%                      | 0%          |
| %3f skip        | 0%      | 0%                    | 0%                      | 0%          | 0%      | 0%                    | 0%                      | 0%          |

{% include direct.html %}

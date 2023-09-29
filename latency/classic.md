---
sort: 4
---
# Wii Classic Controller
Latency of an input from a Wii Classic Controller

Measured by connecting it to the microcontroller in question, and then sending a signal to toggle the "A" button, and timing how long it takes that signal to reach a target device over USB.

| Metric          | Value   | Value                 | Value                   | Value       | Value            | Value   | Value                 | Value                   | Value       |
| --------------- | ------- | --------------------- | ----------------------- | ----------- | ---------------- | ------- | --------------------- | ----------------------- | ----------- |
| microcontroller | Pi Pico | Sparkfun Pro Micro 5V | Sparkfun Pro Micro 3.3V | Arduino Uno | raphnet wusbmote | Pi Pico | Sparkfun Pro Micro 5V | Sparkfun Pro Micro 3.3V | Arduino Uno |
| firmware        | new     | new                   | new                     | new         |                  | old     | old                   | old                     | old         |
| poll rate       | 1 ms    | 1 ms                  | 1 ms                    | 1 ms        | 1 ms             | 1 ms    | 1 ms                  | 1 ms                    | 1 ms        |
| min             | 0.62 ms | 0.53 ms               | 0.62 ms                 | 1.26 ms     | 0.51 ms          | 0.54 ms | 0.92 ms               | 1.47 ms                 | 1.4 ms      |
| max             | 2.66 ms | 2.94 ms               | 3.7 ms                  | 2.58 ms     | 2.47 ms          | 3.47 ms | 2.93 ms               | 8.72 ms                 | 5.36 ms     |
| avg             | 1.11 ms | 1.5 ms                | 1.89 ms                 | 1.81 ms     | 1.46 ms          | 2.04 ms | 1.91 ms               | 3.8 ms                  | 2.74 ms     |
| stddev          | 0.39 ms | 0.5 ms                | 0.61 ms                 | 0.27 ms     | 0.44 ms          | 0.65 ms | 0.44 ms               | 1.68 ms                 | 0.79 ms     |
| %on time        | 94%     | 92%                   | 89%                     | 90%         | 92%              | 88      | 89                    | 78                      | 84          |
| %1f skip        | 6%      | 8%                    | 11%                     | 10%         | 8%               | 12      | 11                    | 22                      | 16          |
| %2f skip        | 0%      | 0%                    | 0%                      | 0%          | 0%               | 0       | 0                     | 0                       | 0           |
| %3f skip        | 0%      | 0%                    | 0%                      | 0%          | 0%               | 0       | 0                     | 0                       | 0           |

{% include classic.html %}

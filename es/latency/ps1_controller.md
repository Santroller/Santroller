---
sort: 8
---

# PS1 Controller

Latency of an input from a PS1 Controller.

Measured by connecting it to the microcontroller in question, and then sending a signal to toggle the right trigger input, and timing how long it takes that signal to reach a target device over USB.

| microcontroller | Pi Pico | Sparkfun Pro Micro 5V | Sparkfun Pro Micro 3.3V | Arduino Uno | Pi Pico | Sparkfun Pro Micro 5V | Sparkfun Pro Micro 3.3V | Arduino Uno |
| :-------------- | :------ | :-------------------- | :---------------------- | :---------- | :------ | :-------------------- | :---------------------- | :---------- |
| firmware        | new     | new                   | new                     | new         | old     | old                   | old                     | old         |
| poll rate       | 1 ms    | 1 ms                  | 1 ms                    | 1 ms        | 1 ms    | 1 ms                  | 1 ms                    | 1 ms        |
| min             | 0.46 ms | 0.48 ms               | 0.66 ms                 | 1.93 ms     | 0.55 ms | 1.1 ms                | 2.07 ms                 | 1.46 ms     |
| max             | 1.94 ms | 2.13 ms               | 2.62 ms                 | 3.07 ms     | 3.21 ms | 3.23 ms               | 4.7 ms                  | 3.11 ms     |
| avg             | 0.97 ms | 1.14 ms               | 1.46 ms                 | 2.43 ms     | 1.29 ms | 2.34 ms               | 3.4 ms                  | 2.26 ms     |
| stddev          | 0.32 ms | 0.35 ms               | 0.39 ms                 | 0.34 ms     | 0.49 ms | 0.42 ms               | 0.56 ms                 | 0.37 ms     |
| %on time        | 95%     | 94%                   | 92%                     | 86%         | 93%     | 87%                   | 80%                     | 87%         |
| %1f skip        | 5%      | 6%                    | 8%                      | 14%         | 7%      | 13%                   | 20%                     | 13%         |
| %2f skip        | 0%      | 0%                    | 0%                      | 0%          | 0%      | 0%                    | 0%                      | 0%          |
| %3f skip        | 0%      | 0%                    | 0%                      | 0%          | 0%      | 0%                    | 0%                      | 0%          |

Note that in the below graph, you can click on legends to show and hide different controllers when comparing.

{% include ps1.html %}

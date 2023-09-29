---
sort: 10
---

# PS2 SG

Latency of an input from a PS1 Controller.

Measured by connecting it to the microcontroller in question, and then sending a signal to toggle the strum up input, and timing how long it takes that signal to reach a target device over USB.
Note that the console only polls this controller every 10ms or so. The developers of the controller took this into account, and the controller will lock up if it is polled too quickly.

| microcontroller | Pi Pico  | Sparkfun Pro Micro 5V | Sparkfun Pro Micro 3.3V | Arduino Uno | Pi Pico  | Sparkfun Pro Micro 5V | Sparkfun Pro Micro 3.3V | Arduino Uno |
| :-------------- | :------- | :-------------------- | :---------------------- | :---------- | :------- | :-------------------- | :---------------------- | :---------- |
| firmware        | new      | new                   | new                     | new         | old      | old                   | old                     | old         |
| controller      | PS2 SG   | PS2 SG                | PS2 SG                  | PS2 SG      | PS2 SG   | PS2 SG                | PS2 SG                  | PS2 SG      |
| poll rate       | 1 ms     | 1 ms                  | 1 ms                    | 1 ms        | 1 ms     | 1 ms                  | 1 ms                    | 1 ms        |
| min             | 3.54 ms  | 4.02 ms               | 4.14 ms                 | 6.04 ms     | 3.92 ms  | 4.82 ms               | 5.2 ms                  | 1.77 ms     |
| max             | 10.06 ms | 12.36 ms              | 10.84 ms                | 12.96 ms    | 10.23 ms | 13.48 ms              | 11.46 ms                | 12.07 ms    |
| avg             | 6.68 ms  | 7.68 ms               | 7.39 ms                 | 9.66 ms     | 6.96 ms  | 8.77 ms               | 8.4 ms                  | 8.55 ms     |
| stddev          | 1.6 ms   | 1.8 ms                | 1.69 ms                 | 1.77 ms     | 1.63 ms  | 1.89 ms               | 1.62 ms                 | 1.75 ms     |
| %on time        | 60%      | 54%                   | 56%                     | 42%         | 58%      | 47%                   | 50%                     | 49%         |
| %1f skip        | 40%      | 46%                   | 44%                     | 58%         | 42%      | 53%                   | 50%                     | 51%         |
| %2f skip        | 0%       | 0%                    | 0%                      | 0%          | 0%       | 0%                    | 0%                      | 0%          |
| %3f skip        | 0%       | 0%                    | 0%                      | 0%          | 0%       | 0%                    | 0%                      | 0%          |

{% include ps2_sg.html %}

---
sort: 1
---

# Direct

| Results         |         |                       |                         |             |         |                       |                         |             |
| :-------------- | ------- | --------------------- | ----------------------- | ----------- | ------- | --------------------- | ----------------------- | ----------- |
| firmware        | new     | new                   | new                     | new         | old     | old                   | old                     | old         |
| microcontroller | Pi Pico | Sparkfun Pro Micro 5V | Sparkfun Pro Micro 3.3V | Arduino Uno | Pi Pico | Sparkfun Pro Micro 5V | Sparkfun Pro Micro 3.3V | Arduino Uno |
| controller      | direct  | direct                | direct                  | direct      | direct  | direct                | direct                  | direct      |
| poll rate       | 1 ms    | 1 ms                  | 1 ms                    | 1 ms        | 1 ms    | 1 ms                  | 1ms                     | 1 ms        |
| min             | 0.53 ms | 0.53 ms               | 0.53 ms                 | 0.48 ms     | 1.32 ms | 0.54 ms               | 0.73 ms                 | 0.92 ms     |
| max             | 1.34 ms | 1.34 ms               | 1.35 ms                 | 1.3 ms      | 2.15 ms | 1.36 ms               | 1.97 ms                 | 1.96 ms     |
| avg             | 0.84 ms | 0.84 ms               | 0.84 ms                 | 0.8 ms      | 1.79 ms | 0.86 ms               | 1.26 ms                 | 1.42 ms     |
| stddev          | 0.24 ms | 0.24 ms               | 0.24 ms                 | 0.25 ms     | 0.22 ms | 0.25 ms               | 0.31 ms                 | 0.3 ms      |
| %on time        | 96%     | 96%                   | 96%                     | 96%         | 90%     | 96%                   | 93%                     | 92%         |
| %1f skip        | 4%      | 4%                    | 4%                      | 4%          | 10%     | 4%                    | 7%                      | 8%          |
| %2f skip        | 0%      | 0%                    | 0%                      | 0%          | 0%      | 0%                    | 0%                      | 0%          |
| %3f skip        | 0%      | 0%                    | 0%                      | 0%          | 0%      | 0%                    | 0%                      | 0%          |

<!-- [![Graph](/assets/images/results/direct.png)](/assets/images/results/direct.png) -->
{% include direct.html %}

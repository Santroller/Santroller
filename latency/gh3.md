---
sort: 5
---

# Wii GH3 Guitar

Latency of an input from a Wii GH3 Guitar.

Measured by connecting it to the microcontroller in question, and then sending a signal to toggle the Strum Up input, and timing how long it takes that signal to reach a target device over USB.

| Results         |         |                  |         |
| :-------------- | ------- | ---------------- | ------- |
| microcontroller | Pi Pico | raphnet wusbmote | Pi Pico |
| firmware        | new     |                  | old     |
| poll rate       | 1 ms    | 1 ms             | 1 ms    |
| min             | 0.48 ms | 0.51 ms          | 0.55 ms |
| max             | 2.17 ms | 2.41 ms          | 3.38 ms |
| avg             | 1.1 ms  | 1.41 ms          | 2.06 ms |
| stddev          | 0.38 ms | 0.43 ms          | 0.66 ms |
| %on time        | 94%     | 92%              | 88%     |
| %1f skip        | 6%      | 8%               | 12%     |
| %2f skip        | 0%      | 0%               | 0%      |
| %3f skip        | 0%      | 0%               | 0%      |

{% include gh3.html %}

---
sort: 6
---

# Wii GHWT Guitar

Latency of an input from a Wii GHWT Guitar.

Measured by connecting it to the microcontroller in question, and then sending a signal to toggle the Strum Up input, and timing how long it takes that signal to reach a target device over USB.

| microcontroller | Pi Pico | raphnet wusbmote | Pi Pico |
| :-------------- | :------ | :--------------- | :------ |
| firmware        | new     |                  | old     |
| poll rate       | 1 ms    | 1 ms             | 1 ms    |
| min             | 0.58 ms | 0.76 ms          | 0.8 ms  |
| max             | 2.57 ms | 2.71 ms          | 3.82 ms |
| avg             | 1.43 ms | 1.72 ms          | 2.34 ms |
| stddev          | 0.39 ms | 0.42 ms          | 0.63 ms |
| %on time        | 92%     | 90%              | 87%     |
| %1f skip        | 8%      | 10%              | 13%     |
| %2f skip        | 0%      | 0%               | 0%      |
| %3f skip        | 0%      | 0%               | 0%      |

{% include wt.html %}

---
sort: 7
---

# Wii GH5 Guitar

Latency of an input from a Wii GH5 Guitar. Note that the GH5 guitars have seperate microcontrollers for handling the neck inputs and other inputs.
This is why there are two seperate sets of data here. Note that the Fret inputs below are measured going via the main PCB, you can go [here](https://santroller.tangentmc.net/latency/gh5_neck_direct_santroller.html) to get the latency information when the main board is bypassed.

Measured by connecting it to the microcontroller in question, and then sending a signal to toggle the Green Fret or Strum Up inputs, and timing how long it takes that signal to reach a target device over USB.

| microcontroller | Pi Pico | raphnet wusbmote | Pi Pico |
| :-------------- | :------ | :--------------- | :------ |
| firmware        | new     |                  | old     |
| controller      | Strum   | Strum            | Strum   |
| poll rate       | 1 ms    | 1 ms             | 1 ms    |
| min             | 0.83 ms | 0.76 ms          | 0.54 ms |
| max             | 5.59 ms | 5.02 ms          | 6.12 ms |
| avg             | 2.78 ms | 2.88 ms          | 3.56 ms |
| stddev          | 0.92 ms | 0.93 ms          | 1.06 ms |
| %on time        | 84%     | 83%              | 79%     |
| %1f skip        | 16%     | 17%              | 21%     |
| %2f skip        | 0%      | 0%               | 0%      |
| %3f skip        | 0%      | 0%               | 0%      |

| microcontroller | Pi Pico  | raphnet wusbmote | Pi Pico |
| :-------------- | :------- | :--------------- | :------ |
| firmware        | new      |                  | old     |
| controller      | Frets    | Frets            | Frets   |
| poll rate       | 1 ms     | 1 ms             | 1 ms    |
| min             | 2.6 ms   | 2.77 ms          | 3.49 ms |
| max             | 10.24 ms | 10.37 ms         | 11.4 ms |
| avg             | 6.29 ms  | 6.59 ms          | 7.19 ms |
| stddev          | 1.45 ms  | 1.48 ms          | 1.58 ms |
| %on time        | 62%      | 61%              | 57%     |
| %1f skip        | 38%      | 39%              | 43%     |
| %2f skip        | 0%       | 0%               | 0%      |
| %3f skip        | 0%       | 0%               | 0%      |

{% include gh5.html %}

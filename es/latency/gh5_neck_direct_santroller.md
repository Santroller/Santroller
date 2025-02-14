---
sort: 14
---

# GH5 Neck

Latency of an input from a GH5 Guitar Neck. Note that this is connected directly to the microcontroller, not via the original mainboard.

Measured by connecting it to the microcontroller in question, and then sending a signal to toggle the Green Fret, and timing how long it takes that signal to reach a target device over USB.

| microcontroller | Pi Pico         |
| :-------------- | :-------------- |
| firmware        | new             |
| controller      | GH5 Guitar Neck |
| poll rate       | 1 ms            |
| min             | 0.79 ms         |
| max             | 6.41 ms         |
| avg             | 3.55 ms         |
| stddev          | 1.28 ms         |
| %on time        | 79%             |
| %1f skip        | 21%             |
| %2f skip        | 0%              |
| %3f skip        | 0%              |

Note that in the below graph, you can click on legends to show and hide different controllers when comparing.

{% include gh5_neck.html %}

---
sort: 12
---

# Peripheral

Latency of a digital input sent from another Pi Pico in [peripheral mode](https://santroller.tangentmc.net/wiring_guides/peripheral.html).

Measured by connecting the peripheral to the main Pi Pico, toggling a digital pin on the peripheral, and timing how long it takes that signal to reach a target device over USB from the main Pi Pico.

| microcontroller  | Pi Pico                    |
| :--------------- | :------------------------- |
| firmware         | new                        |
| controller       | Pi Pico in Peripheral mode |
| peripheral input | direct                     |
| poll rate        | 1 ms                       |
| min              | 0.48 ms                    |
| max              | 1.64 ms                    |
| avg              | 0.84 ms                    |
| stddev           | 0.26 ms                    |
| %on time         | 96%                        |
| %1f skip         | 4%                         |
| %2f skip         | 0%                         |
| %3f skip         | 0%                         |

{% include peripheral.html %}

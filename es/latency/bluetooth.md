---
sort: 3
---

# Bluetooth (between Pi Picos)

Latency of [bluetooth mode](https://santroller.tangentmc.net/tool/bluetooth.html).

Measured by building a transmitter and receiver pair, toggling inputs on the transmitter, and timing how long it takes to receive that input over usb via the receiver.

| controller      | Direct    | Wii Classic Controller | PS1 Controller | PS2 SG    |
| :-------------- | :-------- | :--------------------- | :------------- | :-------- |
| firmware        | new       | new                    | new            | new       |
| microcontroller | Pi Pico   | Pi Pico                | Pi Pico        | Pi Pico   |
| communication   | Bluetooth | Bluetooth              | Bluetooth      | Bluetooth |
| poll rate       | 1 ms      | 1 ms                   | 1 ms           | 1 ms      |
| min             | 3.53 ms   | 0.92 ms                | 3.96 ms        | 7.56 ms   |
| max             | 12.42 ms  | 13.87 ms               | 16.95 ms       | 20.96 ms  |
| avg             | 8.06 ms   | 8.84 ms                | 9.07 ms        | 14.11 ms  |
| stddev          | 2.23 ms   | 2.24 ms                | 2.42 ms        | 2.78 ms   |
| %on time        | 52%       | 47%                    | 46%            | 16%       |
| %1f skip        | 48%       | 53%                    | 54%            | 82%       |
| %2f skip        | 0%        | 0%                     | 0%             | 2%        |
| %3f skip        | 0%        | 0%                     | 0%             | 0%        |

Note that in the below graph, you can click on legends to show and hide different controllers when comparing.

{% include bluetooth.html %}

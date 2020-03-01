Shared
------
| Command              | Char | Args           | Return   | Desc                                                                   |
| -------------------- | ---- | -------------- | -------- | ---------------------------------------------------------------------- |
| Start Config         | s    |                |          | Halt controller processing                                             |
| Apply Config         | a    |                |          | Either restart usb or the MCU                                          |
| Write Config value   | w    | index value... |          |                                                                        |
| read config value    | r    | index          | value... |                                                                        |
| read controller info | i    | index          | value    | wii ext info, version, signature, processor types / board variant, etc |
| Jump bootloader      | e    |                |          |                                                                        |

UNO 
----
| Command                | Char | Args | Return | Desc         |
| ---------------------- | ---- | ---- | ------ | ------------ |
| Jump to usb bootloader | j    |      |        | (16u2 / 8u2) |

Changes
-----

* the keys can read both negative and positive at the same time.
* send axis inverting at the same time as the axis itself

The UNO uses PPP like packet frames to encapsulate serial data. This allows for making it easy to receive both serial and controller data at the same time.
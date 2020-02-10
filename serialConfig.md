Everything is expressed as ascii
Shared
------
| Command              | Char | Args        | Return | Desc                                                                   |
| -------------------- | ---- | ----------- | ------ | ---------------------------------------------------------------------- |
| Start Config         | s    |             |        | Halt controller processing                                             |
| Apply Config         | a    |             |        | Either restart usb or the MCU                                          |
| Write Config value   | w    | index value |        |                                                                        |
| read config value    | r    | index       | value  |                                                                        |
| read controller info | i    | index       | value  | wii ext info, version, signature, processor types / board variant, etc |
| Jump bootloader      | e    |             |        |                                                                        |

UNO 
----
| Command                | Char | Args | Return | Desc         |
| ---------------------- | ---- | ---- | ------ | ------------ |
| Jump to usb bootloader | j    |      |        | (16u2 / 8u2) |

Ideas
-----
Move things like cpu_freq, version and signature out of the eeprom, and have them just be easily retrievable (this should be rather straightforward)

the keys can read both negative and positive at the same time.


## Linux evdev identification

Native USB HID gamepad interfaces append a nine-byte vendor-specific descriptor
after their endpoint descriptors: `09 FF 53 41 4E 54 02 TT CC` (hexadecimal).
This contains the length (9), descriptor type (`FF`), `SANT` signature, protocol
version (2), instrument subtype (`TT`) and capability flags (`CC`). Hosts can
read Linux's cached sysfs USB `descriptors` attribute without hidraw permissions
or USB transfers, associating the identity with the preceding interface.
Identity is per interface so composite devices can expose different instrument
profiles. Interface/profile names, HID report descriptors and input/output
report formats are unchanged.

The descriptor contract can be checked after CMake generates the protobuf headers:

```sh
c++ -std=c++17 -Iinclude -Ibuild/proto -Ilib/nanopb tests/santroller_usb_identity.cpp -o /tmp/santroller_usb_identity
/tmp/santroller_usb_identity
```
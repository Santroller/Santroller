# Host tests

Unit tests for firmware logic, built natively with GoogleTest. Nothing here needs a Pico.

```sh
cmake -S tests/host -B build-tests -G Ninja
cmake --build build-tests
ctest --test-dir build-tests --output-on-failure
```

An installed GoogleTest is used when there is one, otherwise it is downloaded. Tests build with ASan and
UBSan by default; pass `-DSANTROLLER_TESTS_SANITIZE=OFF` to turn them off. The only submodule needed is
`lib/tinyusb`, for its HID / MIDI definitions.

## Layout

- `unit/` – tests for code with no hardware dependencies, all in the `santroller_tests` executable
  (listed in `CMakeLists.txt`).
- `cmake/<area>.cmake` – an area that needs real firmware sources compiled against fakes gets its own
  executable here. Every file in `cmake/` is included automatically; reuse `santroller_headers`,
  `santroller_proto` and `santroller_hidparser` from the main `CMakeLists.txt`.
- `support/` – shared test helpers, plus fakes for SDK / TinyUSB / BTstack functions. Fakes for an area go
  in `support/<area>/` so areas don't step on each other.

## Writing tests

Prefer testing pure logic. When firmware logic is tangled with hardware or USB / Bluetooth stacks, move
the logic into a header-only class or function under `include/protocols/` (like `hid_keyboard.hpp` and
`ble_midi.hpp`) and keep the device class as a thin wrapper around it. Only fall back to compiling a
firmware `.cpp` against fakes when extracting the logic isn't practical.

Expected values come from the protocol, spec or the device's real behaviour, never from just running the
code and copying what it printed.

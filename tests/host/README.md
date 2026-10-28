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

The Xbox One legacy adapter always supports gamepads and uses GH7 output (`0x22`) to select guitar reports
(`0x21`) for `LiveGuitar` profiles alongside standard navigation (`0x20`) and Guide virtual-key events.
QUIESCE stops guitar reports while keeping gamepad navigation; Rock Band 4 device-info
requests select the legacy layout instead. GIP device tests cover these transitions and unchanged-input
resends. Guitar reports are polled even without input changes; navigation reports are change-only.
The navigation layout follows PlasticBand's Xbox One six-fret documentation: White 2/3 and GHTV are
excluded, and strum uses signed left-stick Y. Older configs' retired gamepad-support option is ignored;
the adapter always advertises the gamepad-capable descriptor.
When Xbox One mode is forced by a profile, it uses the native identity of the first reporting profile,
even with WLA enabled or several players attached. This lets Fortnite Festival on PC see a standard guitar.
Forced mode overrides sticky WLA identity; idle WLA retention remains unforced-only.
With WLA enabled, an existing Xbox One adapter is also reused when no profiles match, without adding a
config interface or re-enumerating. This preserves console authentication while the authentication
gamepad is unplugged and a guitar is connected. Config-loader and GIP tests cover the idle gap,
keep-alives, and subsequent GHL input without another authentication.

## Writing tests

Prefer testing pure logic. When firmware logic is tangled with hardware or USB / Bluetooth stacks, move
the logic into a header-only class or function under `include/protocols/` (like `hid_keyboard.hpp` and
`ble_midi.hpp`) and keep the device class as a thin wrapper around it. Only fall back to compiling a
firmware `.cpp` against fakes when extracting the logic isn't practical.

Expected values come from the protocol, spec or the device's real behaviour, never from just running the
code and copying what it printed.

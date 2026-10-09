# Config fixtures from the config tool

These are configs encoded by the config tool's own protobufjs module, exactly as `SettingsContext.ts`
uploads them. `configurator_fixture_test.cpp` uploads each one through `ConfigStorage` in 63 byte reports,
loads it with `ConfigLoader`, and checks every value, so the tool's and the firmware's protos, CRC32 or
upload framing can't drift apart unnoticed.

For each fixture `<name>`:

- `<name>.info.bin` – the length delimited `ConfigInfo` the tool puts in its `ReportIdConfigInfo` report
  (the test zero pads it to 63 bytes like the tool does)
- `<name>.data.bin` – the encoded `Config` followed by the encoded `AuxConfigBlock`, the data the tool
  sends in `ReportIdConfig` reports

| Fixture             | What it is                                                                    |
| ------------------- | ----------------------------------------------------------------------------- |
| `representative`    | Devices, two profiles with assignments, mappings and LEDs, GUI labels, an aux block |
| `empty`             | A fresh device in the tool: nothing configured, so no data at all             |
| `long_profile_name` | A profile name longer than the firmware's 31 bytes, as tools before the limit could send |

## Regenerating

From the config tool's checkout (this repo's parent, with `Santroller` as its submodule):

```sh
yarn install
yarn build_proto
node Santroller/tests/host/config/fixtures/encode_fixtures.mjs
```

`encode_fixtures.mjs` takes the path to the tool's generated `config.js` as an optional argument, for when
this repo isn't checked out inside the tool. The output only changes when the protos or the values in the
script change; when you change the values, change the expectations in `configurator_fixture_test.cpp` to
match and commit the new `.bin` files with them.

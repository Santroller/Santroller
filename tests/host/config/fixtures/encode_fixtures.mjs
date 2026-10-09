// Encodes the config fixtures with the config tool's own protobufjs module, the same way
// SettingsContext.ts uploads a config. See README.md in this folder.
//
//   node encode_fixtures.mjs [path/to/SettingsContext/config.js]
//
// The default path is the config tool's, when this repo is checked out as its Santroller submodule.
import { createRequire, registerHooks } from 'node:module';
import { writeFileSync } from 'node:fs';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';

const here = dirname(fileURLToPath(import.meta.url));
const configModule = resolve(
  process.argv[2] ?? join(here, '../../../../../src/components/SettingsContext/config.js')
);

// The generated module does `import * as $protobuf from "protobufjs/minimal"`, which only works through a
// bundler's CommonJS interop. Hand it the tool's own protobufjs as an ES module instead.
globalThis.__santrollerProtobuf = createRequire(configModule)('protobufjs/minimal');
const SHIM = 'santroller-fixtures:protobufjs-minimal';
registerHooks({
  resolve(specifier, context, next) {
    return specifier === 'protobufjs/minimal' ? { url: SHIM, shortCircuit: true } : next(specifier, context);
  },
  load(url, context, next) {
    if (url !== SHIM) {
      return next(url, context);
    }
    const source = `const p = globalThis.__santrollerProtobuf;
export default p;
export const roots = p.roots, Reader = p.Reader, Writer = p.Writer, util = p.util;`;
    return { format: 'module', source, shortCircuit: true };
  },
});
const { proto } = await import(pathToFileURL(configModule).href);

// src/CRC32.ts
class CRC32 {
  crc32_table = [
    0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac, 0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
    0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c, 0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c,
  ];

  calculate(bytes) {
    let state = ~0;
    for (const data of bytes) {
      let tbl_idx = 0;
      tbl_idx = state ^ (data >>> (0 * 4));
      state = this.crc32_table[tbl_idx & 0x0f] ^ (state >>> 4);
      tbl_idx = state ^ (data >>> (1 * 4));
      state = this.crc32_table[tbl_idx & 0x0f] ^ (state >>> 4);
    }
    return ~state;
  }
}

// SettingsContext.ts
const magic = 0xd2f1e365;

// buildConfigBuffer and writeConfig in SettingsContext.ts
function encodeUpload(config, aux) {
  const bufferMain = proto.Config.encode(config).finish();
  const bufferAux = proto.AuxConfigBlock.encode(aux).finish();
  const buffer = new Uint8Array(bufferMain.length + bufferAux.length);
  buffer.set(bufferMain, 0);
  buffer.set(bufferAux, bufferMain.length);
  const crc = new CRC32().calculate(buffer);
  const info = proto.ConfigInfo.encode(
    proto.ConfigInfo.create({
      dataSize: buffer.length,
      dataCrc: crc,
      auxSize: bufferAux.length,
      mainSize: bufferMain.length,
      magic,
    })
  )
    .ldelim()
    .finish();
  return { info, data: buffer };
}

function write(name, config, aux) {
  const { info, data } = encodeUpload(config, aux);
  writeFileSync(join(here, `${name}.info.bin`), info);
  writeFileSync(join(here, `${name}.data.bin`), data);
  console.log(`${name}: ${data.length} bytes`);
}

const gpio = (pin, pinMode, analog = false) => ({ gpio: { pin, pinMode, analog } });

// Every value here is checked by configurator_fixture_test.cpp, so keep the two in step
const representative = proto.Config.create({
  devices: [
    {
      deviceid: 1,
      usbHost: {
        firstPin: 20,
        enable5v: true,
        dmFirst: false,
        mappingMode: proto.MappingMode.PerInput,
        noteHoldTime: 50,
      },
    },
    { deviceid: 2, ws2812: { pin: 23, type: proto.WS2812Type.Ws2812Grb, count: 5 } },
    { deviceid: 3, cycle: { type: proto.CycleType.custom, values: [0, 4, -1, 1000] } },
    {
      deviceid: 4,
      mpr121: {
        i2c: { block: 1, sda: 18, scl: 19, clock: 400000 },
        touchpadCount: 12,
        ddrPins: 0,
        enablePins: 0,
      },
    },
    { deviceid: 5, toggle: {} },
  ],
  profiles: [
    {
      opts: {
        uid: 0x1234,
        faceButtonMappingMode: proto.FaceButtonMappingMode.LegendBased,
        name: 'Guitar',
        deviceToEmulate: proto.SubType.GuitarHeroGuitar,
        queueInputs: true,
        dequeueInterval100us: 20,
        deviceSlotIdVersion: 1,
        ps3OnRpcs3: false,
      },
      assignments: [
        {
          assignments: [
            { usbType: proto.SubType.GuitarHeroGuitar },
            { consoleType: { consoleType: proto.ConsoleType.ConsoleXbox360, xinputOnWindows: true } },
          ],
        },
      ],
      mappings: [
        {
          mapping: { gamepadButton: proto.GamepadButtonType.Gamepad_A },
          input: gpio(2, proto.PinMode.PullUp),
          debounce: 5,
        },
        {
          mapping: { gamepadAxis: proto.GamepadAxisType.Gamepad_LeftStickX },
          input: gpio(26, proto.PinMode.Floating, true),
          min: -32767,
          max: 32767,
          center: 0,
          deadzone: 3000,
        },
        {
          mapping: { gamepadButton: proto.GamepadButtonType.Gamepad_Start },
          input: { fixed: { value: -5 } },
        },
      ],
      leds: [
        {
          device: { gpio: { pin: 25, analog: false } },
          mapping: { staticMapping: {} },
        },
        {
          device: {
            rgb: {
              deviceId: 2,
              startR: 255,
              startG: 0,
              startB: 0,
              startW: 0,
              endR: 0,
              endG: 0,
              endB: 255,
              endW: 0,
              activeLed: [0, 1, 4],
              hasStart: true,
            },
          },
          mapping: { patternMapping: { pattern: proto.RgbPatternType.PatternFade, speed: 3, brightness: 200 } },
        },
      ],
    },
    {
      opts: {
        uid: 7,
        faceButtonMappingMode: proto.FaceButtonMappingMode.PositionBased,
        name: 'Pad',
        deviceToEmulate: proto.SubType.Gamepad,
      },
      assignments: [
        { assignments: [{ usbType: proto.SubType.Gamepad }] },
        { assignments: [{ bluetooth: proto.BluetoothMode.BTStandard }] },
      ],
      mappings: [
        {
          mapping: { gamepadButton: proto.GamepadButtonType.Gamepad_X },
          input: { key: { deviceid: 1, key: 4 } },
        },
      ],
    },
  ],
  // The firmware never reads these, but has to step over them
  guiConfig: [
    { deviceid: 1, label: { label: 'USB port', pin: 20, showToCustomer: true } },
    { deviceid: 0, seller: { logo: Uint8Array.from({ length: 300 }, (_, i) => i & 0xff), name: 'Seller' } },
  ],
  syncCalibrations: true,
  inactivity: { sleepTimeoutSec: 600, wakePin: 15, wakeActiveHigh: false, ledTimeoutSec: 30 },
});

const representativeAux = proto.AuxConfigBlock.create({
  states: [{ id: 3, state: 2 }],
  toggleStates: [{ id: 5, state: true }],
  bluetoothStates: [
    {
      id: 0,
      macAddress: Uint8Array.from([0xaa, 0xbb, 0xcc, 0x01, 0x02, 0x03]),
      name: 'Pad',
      ble: true,
      subtype: proto.SubType.RockBandGuitar,
      controllerType: proto.BtControllerType.BtControllerTypePS4,
      vid: 0x054c,
      pid: 0x09cc,
      linkKey: Uint8Array.from({ length: 16 }, (_, i) => i + 1),
    },
  ],
  tlvEntries: [{ tag: 0x42544c00, value: Uint8Array.from([7, 8, 9]) }],
});

write('representative', representative, representativeAux);

// A fresh device in the tool: nothing configured (initialConfig in SettingsContext.ts)
write(
  'empty',
  proto.Config.create({ devices: [], profiles: [] }),
  proto.AuxConfigBlock.create({ states: [], bluetoothStates: [], tlvEntries: [] })
);

// What config tools before the profile name limit (profileName.ts) could send: a name longer than the
// firmware's 31 bytes, which the firmware now cuts short
write(
  'long_profile_name',
  proto.Config.create({
    profiles: [
      {
        opts: {
          uid: 1,
          faceButtonMappingMode: proto.FaceButtonMappingMode.LegendBased,
          name: 'My favourite guitar for Clone Hero',
          deviceToEmulate: proto.SubType.GuitarHeroGuitar,
        },
        mappings: [
          {
            mapping: { gamepadButton: proto.GamepadButtonType.Gamepad_A },
            input: gpio(2, proto.PinMode.PullUp),
          },
        ],
      },
    ],
  }),
  proto.AuxConfigBlock.create({})
);

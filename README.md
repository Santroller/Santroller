# The Santroller Platform
The Santroller platform provides the ability to program micro controllers that emulate various rhythm game controllers for computers and various consoles. This is done using the Santroller Configurator, which can program various microcontrollers to emulate different controllers.

Documentation can be found at https://santroller.tangentmc.net/

## Firmware update integrity

OTA images include a 256-byte trailer: 224 zero bytes followed by the SHA-256 digest of
the preceding application bytes. Firmware verifies the staged image before marking it
valid and rebooting into the update. Raw application binaries without this trailer are
not valid OTA inputs; use the generated `santroller_fota_image.bin` or the configurator's
packaged OTA image. Automatic secondary-Pico updates generate the same trailer from
the running application.

Older firmware without verification can install the new image, but cannot validate that
first transition. Verification applies to subsequent updates received by the new firmware.
This detects accidental corruption, not image authenticity or hardware compatibility.
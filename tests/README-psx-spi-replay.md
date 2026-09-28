# PS2 SPI replay test

Host-side replay of the reconstructed Wireless PS2 startup sequence.

The command parser is not duplicated here: the test includes
lib/psx_emulation/psx_spi_protocol.h, which is also called directly by the
firmware pio_spi.c IRQ path.

Build:
  cc -std=c11 -Wall -Wextra -O2 tests/psx_spi_replay.c -o /tmp/psx_spi_replay

Run:
  /tmp/psx_spi_replay

Checks the 43 pipeline, 44 01 03 analog selection, 4D, 4F FF FF 03,
73 -> 79 transition, and stateful 45 response.

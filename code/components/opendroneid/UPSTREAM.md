# OpenDroneID decoder

Source: https://github.com/opendroneid/opendroneid-core-c

Pinned commit: `849c03adbe0cdd68997a462fafab397edcfe53f3`.

`opendroneid.c` and `opendroneid.h` come from upstream `libopendroneid/`.
The Apache-2.0 license and original copyright notices are retained. Local changes:

- ESP-IDF CMake integration.
- Include `inttypes.h` and use `PRIu32` in the two timestamp-printing helpers,
  so they compile with the ESP32-C61 RISC-V toolchain's strict format checks.
- Remove trailing whitespace in two header comments.

The decoding routines are unchanged.

This app uses the decoder for Basic ID messages. Transport parsing and length
validation are in `main/rid_protocol.c`. There is no nested Git repository.

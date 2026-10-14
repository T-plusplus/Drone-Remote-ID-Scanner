# Button-triggered Remote ID scanner

Press SCAN to listen for drone Remote ID broadcasts for approximately six seconds.
The OLED shows a countdown, then the Basic ID with the highest received RSSI,
its signal strength in dBm, and the transport (BLE or WiFi). If no valid Basic ID
arrives, it shows `No drones found`. Results stay visible until the next scan.
Holding the button triggers one scan; presses during a scan are ignored.
Every scan starts with an empty result, so old aircraft IDs are never reused.

The receiver listens passively for:

- Bluetooth LE legacy and extended advertisements on the 1M and coded PHYs,
  using the Remote ID service UUID 0xFFFA and app code 0x0D.
- 2.4 GHz Wi-Fi Remote ID vendor beacons and NAN service discovery frames,
  using the OpenDroneID message-pack format.

Bluetooth gets three seconds, followed by three seconds of Wi-Fi. Separating
these phases avoids the SDK's documented unstable simultaneous BLE scan / Wi-Fi
sniffer combination. Wi-Fi channel 6 gets one second; channels 1-5 and 7-11 get
200 ms each. This is a short proof-of-concept search: repeat SCAN if needed.
Radio failures show `Scan error` instead of a misleading empty result.

The selection uses the strongest individual valid Basic ID report in the window.
RSSI is a rough proximity estimate: antenna orientation, transmit power, obstacles,
and the different radio transports affect it. It does not measure physical distance.
`No drones found` means no supported Basic ID was received during that window.
The ESP32-C61 cannot receive 5 GHz Wi-Fi, and this app does not decode proprietary
drone protocols. Aircraft reception still needs testing with a real transmitter.

Serial numbers and registration IDs are displayed as text. UUIDs and session IDs
are displayed as hexadecimal, wrapped across the screen without truncation.
Packet parsing checks lengths and protocol versions before decoding the Basic ID
with the vendored OpenDroneID C library.

## Breadboard settings

| Board label | Function |
| --- | --- |
| `3V3` | OLED VCC |
| `G` | OLED GND and button ground |
| `0` / GPIO0 | OLED SDA |
| `1` / GPIO1 | OLED SCL |
| `2` / GPIO2 | SCAN button input, active low |

The button is debounced for 30 ms and uses an internal pull-up. The external
10 kOhm button resistor may remain fitted or be omitted. No additional I2C
pull-ups are required by default. The app probes addresses 0x3C and 0x3D and uses
ESP-IDF's SSD1306 driver at 100 kHz with a 128 x 32 display and an 8 x 8 font.

## Build and flash

This project is built with ESP-IDF **5.5.5**, targeting ESP32-C61 with 8 MB flash.
The older installed 5.2 SDK does not support this board.

```powershell
Set-Location 'D:\Users\Tyler\Documents\DroneRemoteIDScanner\code'
.\idf.cmd build
.\idf.cmd -p COM3 flash
```

`idf.cmd` activates the SDK at `C:\Users\Tyler\esp\v5.5.5\esp-idf` and tools at
`C:\Users\Tyler\.espressif-c61` in a child Command Prompt. It works with restricted
PowerShell execution policy; no `activate.ps1` or policy change is necessary.
Use `.\idf.cmd ports` to find the port and substitute it for COM3 if needed.
Connect through the board's native ESP32 USB port for the configured USB console.
Optional logs: `.\idf.cmd -p COM3 monitor`; exit with Ctrl+].

The launcher uses `code/build-remote-id/`, including its own generated `sdkconfig`,
so the previous Hello World build's radio settings cannot override scanner defaults.
`sdkconfig.defaults` enables NimBLE observer mode, extended/coded scanning and
radio coexistence. `partitions.csv` provides a 3 MB application partition within
the board's 8 MB flash. Generated build files are ignored by Git.

Override `C61_IDF_PATH`, `C61_TOOLS_PATH`, or `C61_BUILD_PATH` if needed. Another
computer must install ESP-IDF and its tools; they are not included in this repo.

## Validation and bench check

The ESP32-C61 build passes, including bootloader and application partition size
checks. Host tests cover BLE, message packs, Wi-Fi beacon/NAN framing, fragmented
BLE advertisements, strongest-RSSI selection, and 20,000 malformed inputs under
AddressSanitizer. Run them from `code` with `.\tests\run.cmd` (Visual Studio C++
Build Tools and its AddressSanitizer component are required).

The firmware was flashed over COM3 with verified hashes. A startup log from the
actual board confirmed the OLED at 0x3C, Wi-Fi and Bluetooth initialization, and
the ready state. The breadboard operator also confirmed the button-triggered
countdown and `No drones found` result. Live drone reception and strongest-signal
selection among multiple transmitters still need bench testing.

On the breadboard:

1. Reset: `Starting radios`, then `Remote ID ready` / `Press SCAN`.
2. Press SCAN: countdown, then a Remote ID or `No drones found`.
3. Hold SCAN: one scan only. Release and press again for a new scan.
4. Test with a known Remote ID transmitter. Confirm its ID and move it to check
   changes in RSSI; use multiple transmitters to test strongest-signal selection.
5. If the OLED shows `Radio error` or `Scan error`, inspect the optional USB log.

## Source layout and references

- `main/main.c`: button and screen flow.
- `main/oled.c`: SSD1306 initialization and four-line rendering.
- `main/rid_scanner.c`: radios, channel schedule and scan lifecycle.
- `main/rid_protocol.c`: bounded transport parsing and result selection.
- `components/opendroneid/`: pinned decoder, license and provenance.
- `main/fonts/`: public-domain font and provenance.
- `tests/`: host parser and selection tests.

References: [OpenDroneID C library](https://github.com/opendroneid/opendroneid-core-c),
[Espressif coexistence guide](https://docs.espressif.com/projects/esp-idf/en/v5.5.5/esp32c61/api-guides/coexist.html),
and [breadboard wiring](../hardware/wiring/breadboard.md).

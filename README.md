# Drone Remote ID Scanner

An ESP32-C61-DevKitC-1-N8R2 breadboard proof of concept for receiving nearby
drone Remote ID broadcasts and displaying results on an I2C OLED, with one
SCAN pushbutton.

Pressing SCAN starts a six-second passive search: three seconds of Bluetooth LE,
then three seconds of 2.4 GHz Wi-Fi. The OLED displays the Remote ID with the
strongest received signal, or `No drones found` if no valid Basic ID arrives.
Strongest RSSI estimates proximity; it does not establish physical distance.

Current stage: breadboard proof of concept with Remote ID receiver firmware;
see [code setup and supported transports](code/README.md). Reception of actual
drone broadcasts still needs bench validation. Schematics/PCB design and
printable enclosure design remain future work.

## Project structure

```text
DroneRemoteIDScanner/
|-- code/                       ESP-IDF firmware and its build configuration
|-- hardware/
|   |-- wiring/                 Breadboard diagram and connection guide
|   |-- kicad/                  Future schematic and PCB project
|   `-- exports/                Future schematic PDFs and fabrication outputs
|-- datasheets/                 Supplied OLED PDF and future module datasheets
`-- enclosures/
    |-- cad/                    Future editable enclosure designs
    `-- prints/                 Future STL/3MF print files
```

Everything belongs to this single Git repository. Do not initialize nested
repositories in the project folders. No remote host has been configured.
Source files and intentionally published hardware/print exports belong in Git;
temporary build files, tool caches, and local editor state are ignored.

## Breadboard connections

See [the wiring guide](hardware/wiring/breadboard.md) and
[the wiring diagram](hardware/wiring/breadboard.png).

| ESP32-C61 connection | Breadboard connection |
| --- | --- |
| 3V3, J1 pin 1 | MakerFocus OLED VCC |
| GND, J1 pin 15 | OLED GND and button ground |
| GPIO0, J1 pin 7, board label `0` | OLED SDA |
| GPIO1, J1 pin 8, board label `1` | OLED SCL |
| GPIO2, J1 pin 12, board label `2` | Normally-open button to GND; 10 kOhm pull-up to 3V3 |

Power the ESP32 through USB for this prototype. Wire by the OLED header labels,
not by an assumed left-to-right pin order. The user identified the OLED as
MakerFocus ASIN B079BN2J8V (0.91 inch, SSD1306, 128 x 32). MakerFocus specifies
3.3-5 V input. Use 3.3 V and start without extra I2C pull-up resistors; onboard
pull-ups are inferred from the manufacturer's component photo, pending a
schematic or continuity check of the actual board.

The firmware parses Remote ID BLE legacy/extended advertisements and 2.4 GHz
Wi-Fi beacons/NAN frames. ESP32-C61 has no 5 GHz Wi-Fi reception. A short scan
can miss broadcasts; `No drones found` means none were decoded in that window.

## Reference sources

- [Espressif board guide, v2.0](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32c61/esp32-c61-devkitc-1/user_guide.html)
- [Espressif board guide, v1.0](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32c61/esp32-c61-devkitc-1/user_guide_v1.0.html)
- [ESP32-C61 datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-c61_datasheet_en.pdf)

# Drone Remote ID Scanner

An ESP32-C61-DevKitC-1-N8R2 breadboard proof of concept for receiving nearby
drone Remote ID broadcasts and displaying results on an I2C OLED, with one
SCAN pushbutton.

Intended behavior: pressing SCAN starts a scan and displays the nearest
aircraft's Remote ID. The method for determining nearest will be decided when
firmware work starts: strongest received signal is a proximity estimate; true
geographic distance needs the aircraft position and the receiver's own position.

Current stage: breadboard proof of concept. The first firmware app lights the
OLED with `Hello, world!` when SCAN is pressed; see [code setup](code/README.md).
Drone reception, schematics/PCB design, and printable enclosure design are future work.

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

The eventual radio implementation must be validated for each intended Remote
ID transport; receiving all nearby drones is not guaranteed. ESP32-C61 provides
2.4 GHz Wi-Fi and Bluetooth LE, with no 5 GHz Wi-Fi reception. This is a future
firmware scope note, not an implemented capability.

## Reference sources

- [Espressif board guide, v2.0](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32c61/esp32-c61-devkitc-1/user_guide.html)
- [Espressif board guide, v1.0](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32c61/esp32-c61-devkitc-1/user_guide_v1.0.html)
- [ESP32-C61 datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-c61_datasheet_en.pdf)

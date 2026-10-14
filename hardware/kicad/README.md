# KiCad prototype schematic

Rev A documents the USB-powered breadboard prototype using the complete
ESP32-C61-DevKitC-1-N8R2 development board, MakerFocus 0.91 inch SSD1306
128 x 32 OLED breakout (ASIN B079BN2J8V), and one SCAN pushbutton.

Open `DroneRemoteIDScanner.kicad_pro` in KiCad 10, then open its schematic.
`DroneRemoteIDScanner.kicad_sch` is the editable circuit. The project-local
`RemoteID.kicad_sym` library and `sym-lib-table` travel with the project;
standard resistor, switch, and power symbols use KiCad 10's installed libraries.
The schematic also embeds all the symbols it uses.

## Circuit

| Net | U1 development board | Other connections |
| --- | --- | --- |
| +3V3 | J1 pin 1, 3V3 | U2 VCC; R1/R2/R3 pin 1 |
| GND | J1 pin 15, G | U2 GND; SW1 pin 2 |
| SDA | J1 pin 7, GPIO0 | U2 SDA; R2 pin 2 |
| SCL | J1 pin 8, GPIO1 | U2 SCL; R3 pin 2 |
| SCAN_N | J1 pin 12, GPIO2 | R1 pin 2; SW1 pin 1 |

Labels with the same name connect electrically even when their wires are
drawn in different sections of the sheet.

| Reference | Part/value | Initial assembly |
| --- | --- | --- |
| U1 | ESP32-C61-DevKitC-1-N8R2, complete development board | Fit |
| U2 | MakerFocus SSD1306 128 x 32 OLED breakout | Fit |
| R1 | 10 kOhm, SCAN input pull-up | Fit |
| SW1 | Normally-open momentary SCAN switch | Fit |
| R2 | 4.7 kOhm, SDA pull-up | DNP |
| R3 | 4.7 kOhm, SCL pull-up | DNP |

R2 and R3 are marked **do not populate** in KiCad, not just in their displayed
values. Start without them. Confirm the OLED's existing pull-ups before fitting
either resistor; the manufacturer's photo suggests onboard 4.7 kOhm pull-ups,
but their net connections have not been measured. All bus pull-ups go to 3.3 V.

SW1 pulls SCAN_N low when pressed. R1 defines the released state before firmware
starts; firmware also enables GPIO2's internal pull-up and debounces for 30 ms.
Use one terminal from each electrically distinct contact group if the physical
switch has four legs. There is no added hardware debounce capacitor.

Power enters through U1's existing USB port and onboard regulator. Leave U1's
J5 jumper fitted for this prototype. U1's 3V3 pin is modeled as a power output;
the ground PWR_FLAG represents the common reference provided by the powered
development board. It is an ERC annotation, not a physical part.

## Symbol scope and PCB work

U1 is an **interface-only development-board symbol**. Its five pin numbers are
the actual **J1 header positions**, not GPIO numbers or bare WROOM module pads.
Unused J1 pins and the entire J3 header are omitted. These five used positions
are common to Espressif's documented v1.0 and v2.0 boards. The symbol does not
recreate the board's regulator, USB circuitry, reset, or RF implementation.

U2's pin identifiers are the signal names `GND`, `VCC`, `SCL`, and `SDA`.
They intentionally do not claim a physical left-to-right header order.
Wire the actual breakout by its printed labels.

All component footprints are unassigned, and no PCB file is created. Before a
carrier-board layout, verify the actual DevKit revision, both header dimensions
and numbering orientation, OLED header order/pitch and mounting details, and
the switch's package/contact arrangement. Replace or extend the interface
symbols with verified physical pad numbering when assigning footprints. A
standalone WROOM board would also need its own power, USB/programming, reset,
boot, decoupling, and antenna-clearance design.

## Validation and exports

Checked with KiCad CLI 10.0.7 on 2026-10-13:

- ERC: **0 errors and 0 warnings**, without custom exclusions.
- Exported XML netlist: all five nets and their component pins match the table.
- Firmware: SDA GPIO0, SCL GPIO1, SCAN GPIO2, and 30 ms debounce match the circuit.
- R2/R3: native KiCad DNP flags verified in the exported netlist.
- Final PDF: rendered with Poppler and visually inspected for legibility and overlap.

These checks verify the schematic's connectivity and presentation. Physical
pull-up measurements and Remote ID reception still need bench validation.

Published previews and the report are in [`../exports/`](../exports/):

- [Schematic PDF](../exports/DroneRemoteIDScanner-schematic.pdf)
- [Schematic SVG](../exports/DroneRemoteIDScanner.svg)
- [ERC report](../exports/DroneRemoteIDScanner-erc.txt)

After editing the schematic, regenerate the previews and report with KiCad's
Export and ERC tools, or run the following from the repository root. Set
`$kicadCli` to your installed CLI; this machine uses the path shown here.

```powershell
$kicadCli = 'D:/Program Files/KiCad/10.0/bin/kicad-cli.exe'
& $kicadCli sch erc hardware/kicad/DroneRemoteIDScanner.kicad_sch --severity-all --exit-code-violations --output hardware/exports/DroneRemoteIDScanner-erc.txt
& $kicadCli sch export pdf hardware/kicad/DroneRemoteIDScanner.kicad_sch --no-background-color --output hardware/exports/DroneRemoteIDScanner-schematic.pdf
& $kicadCli sch export svg hardware/kicad/DroneRemoteIDScanner.kicad_sch --no-background-color --output hardware/exports/
```

## Reference sources

Pin assignments and module specifications rechecked on 2026-10-13:

- [Espressif DevKit v2.0 header table](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32c61/esp32-c61-devkitc-1/user_guide.html#header-block)
- [Espressif DevKit v1.0 header table](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32c61/esp32-c61-devkitc-1/user_guide_v1.0.html#header-block)
- [MakerFocus OLED specifications](https://www.makerfocus.com/products/2pcs-i2c-oled-display-module-0-91-inch-i2c-ssd1306-oled-display-module-1)
- [Breadboard wiring and pull-up evidence](../wiring/breadboard.md)

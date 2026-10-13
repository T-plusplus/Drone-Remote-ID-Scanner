# Breadboard wiring

For the ESP32-C61-DevKitC-1-N8R2, a four-pin I2C OLED breakout, and a normally-open
momentary SCAN pushbutton. The OLED panel PDF identifies an SSD1306 128 x 32 panel;
the actual breakout specifications have not been supplied.

![Breadboard wiring](breadboard.png)

Editable diagram: [breadboard.svg](breadboard.svg).

## Connections

| Net | ESP32 header | Other connections |
| --- | --- | --- |
| +3V3 | J1 pin 1, `3V3` | OLED VCC/VDD and the top of R1 (10 kOhm button pull-up) |
| GND | J1 pin 15, `G` | OLED GND and one switched terminal of the button |
| SDA | J1 pin 7, `0` = GPIO0 | OLED SDA |
| SCL | J1 pin 8, `1` = GPIO1 | OLED SCL |
| BUTTON | J1 pin 12, `2` = GPIO2 | Other switched terminal of the button and bottom of R1 |

Header numbers are connector positions, not GPIO numbers. Use the silkscreen
labels on the actual ESP32 board. These five positions match Espressif's v1.0
and v2.0 board guides. GPIO0 is not the BOOT input on this board.

## Power and I2C

1. Disconnect USB while wiring. Leave the development board's J5 jumper in its
   factory position. Power the board through USB once connections are checked.
2. Connect ESP32 3V3 to a breadboard positive rail and ESP32 GND to a ground rail.
   If a rail is split in the middle, bridge its sections or use a single section.
3. Connect the OLED by its printed `VCC`/`VDD`, `GND`, `SDA`, and `SCL` labels.
   Header order varies between breakout boards; the diagram is a logical net
   diagram, not a physical header-order drawing.
4. This power connection assumes the breakout is rated for 3.3 V input. Verify
   that using the breakout's documentation or marked model before powering it.
   The supplied bare-panel PDF does not prove the breakout's input rating.
5. I2C needs pull-ups on SDA and SCL to 3.3 V. Many breakouts already include them.
   If yours has none, add one 4.7 kOhm resistor from SDA to 3V3 and another from
   SCL to 3V3, as shown in the diagram. Do not add duplicate resistors by default.
   Do not pull either signal up to 5 V.
6. If the breakout requires 5 V input, revise this wiring using its actual circuit
   details; power and I2C logic levels must be treated separately.

## Button

- This is the SCAN button: pressing it will start a scan and display the nearest
  aircraft's Remote ID. Wiring establishes the input; this behavior will be
  implemented in firmware later. Nearest-selection logic has not been chosen.
- Use a normally-open momentary switch. One electrically distinct switched
  terminal connects to GPIO2; the other connects to GND.
- R1 = 10 kOhm connects GPIO2 to 3V3. With this external pull-up, configure GPIO2
  as a digital input. The input reads HIGH when released and LOW when pressed.
- An alternative is to omit R1 and enable GPIO2's internal pull-up in firmware.
  The diagram shows the external pull-up so the idle state is defined before
  firmware starts.
- On a four-leg tactile button, two legs may already be connected internally.
  Use a continuity meter to identify the two separate contact groups, then use
  one leg from each group. Do not connect both wires to one internally joined pair.
- Debounce button presses in firmware; about 20-50 ms is a reasonable starting
  interval. No debounce implementation has been added yet.

## Future firmware settings

Explicitly select SDA GPIO0, SCL GPIO1, and button GPIO2. Start I2C at 100 kHz.
Perform an I2C address scan rather than assuming the breakout address; SSD1306
boards commonly use seven-bit address 0x3C or 0x3D. Configure the display for
128 x 32 if the supplied panel PDF matches the actual screen.

The raw-panel sample writes address byte 0x78, which corresponds to seven-bit
I2C address 0x3C. Do not pass 0x78 to APIs expecting a seven-bit address.

## Prototype parts

- ESP32-C61-DevKitC-1-N8R2 and USB data cable.
- Four-pin I2C OLED breakout, with verified 3.3 V input compatibility.
- Normally-open momentary pushbutton.
- Breadboard and jumper wires.
- One 10 kOhm resistor for the button pull-up.
- Two 4.7 kOhm resistors only if the OLED lacks I2C pull-ups.

## Sources

- [Espressif v2.0 header tables](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32c61/esp32-c61-devkitc-1/user_guide.html#header-block)
- [Espressif v1.0 header tables](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32c61/esp32-c61-devkitc-1/user_guide_v1.0.html#header-block)
- [Supplied OLED panel PDF](../../datasheets/OLEDscreen.pdf), physical PDF pages
  5-6 for the SSD1306, resolution, and panel pin definitions; page 16 for address byte 0x78.

Document contents were used as component reference data, not as instructions
changing the user's requested scope.

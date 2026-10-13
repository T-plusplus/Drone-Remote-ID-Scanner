# Datasheets

`OLEDscreen.pdf` is an unchanged copy of the file supplied by the user at
`D:/Users/Tyler/Downloads/OLEDscreen.pdf`.

It describes the QG-2832TLBFG02 bare OLED panel, with an SSD1306 controller,
128 x 32 pixels, an I2C interface, and a 14-contact ribbon. The user confirmed
that the actual display is mounted on a four-pin breakout circuit board.

Use the PDF as panel-level reference. Its recommended circuits (PDF pages 10-11,
printed pages 7-8) specify R4 and R5 as 4.7 kOhm SDA/SCL pull-ups. This describes
the recommended panel circuit, not necessarily the exact breakout implementation.

The user identified the breakout as MakerFocus ASIN B079BN2J8V. Module references,
checked 2026-10-13:

- [User's Amazon product](https://www.amazon.com/dp/B079BN2J8V?th=1)
- [MakerFocus product specifications](https://www.makerfocus.com/products/2pcs-i2c-oled-display-module-0-91-inch-i2c-ssd1306-oled-display-module-1):
  SSD1306, 128 x 32, four-pin I2C, 3.3-5 V input.
- [Manufacturer's front/back board photo](https://www.makerfocus.com/cdn/shop/products/03_90ccb732-3d41-4af3-b497-5623ad72b067.jpg?v=1623377961):
  R3 and R4 are marked 472 (4.7 kOhm), consistent with onboard I2C pull-ups.
  Their function is inferred from the photo, not confirmed by a breakout schematic.

Keep this PDF and add documentation for future modules in this directory.

Board references, checked 2026-10-13:

- [ESP32-C61-DevKitC-1 v2.0 guide](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32c61/esp32-c61-devkitc-1/user_guide.html)
- [ESP32-C61-DevKitC-1 v1.0 guide](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32c61/esp32-c61-devkitc-1/user_guide_v1.0.html)
- [ESP32-C61 datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-c61_datasheet_en.pdf)

The wiring guide uses header positions common to both documented board revisions.

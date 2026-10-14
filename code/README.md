# Button + OLED hello world

The first breadboard firmware milestone:

1. On startup the OLED is off.
2. Press the SCAN button (GPIO2 to GND).
3. The OLED lights up with `Hello,` and `world!` on two lines.
4. Releasing the button leaves the message visible. Later presses also leave it
   visible. Reset the ESP32 to return to the initial screen-off state.

The button is debounced for 30 ms. An internal button pull-up is enabled, so the
external 10 kOhm button resistor can remain fitted or be omitted. I2C uses the
breakout's expected onboard pull-ups; no extra I2C resistors are added by default.

| Board label | Function |
| --- | --- |
| `3V3` | OLED VCC |
| `G` | OLED GND and button ground |
| `0` / GPIO0 | OLED SDA |
| `1` / GPIO1 | OLED SCL |
| `2` / GPIO2 | SCAN button input, active low |

The app probes I2C addresses 0x3C and 0x3D automatically. It uses ESP-IDF's built-in
SSD1306 driver with 128 x 32 resolution and a tiny greeting font; no external
display library, Arduino framework, or graphics framework is required.

## Build and flash

This app uses ESP-IDF **5.5.5 or newer**, and is set up with **5.5.5**. Production
ESP32-C61 support began with 5.5.2; the previously installed 5.2 SDK does not
support this board.

In PowerShell on this machine, activate the parallel C61 tool installation:

```powershell
Set-Location 'D:\Users\Tyler\Documents\DroneRemoteIDScanner\code'
. .\activate.ps1
idf.py build
```

`activate.ps1` selects the SDK at `C:\Users\Tyler\esp\v5.5.5\esp-idf`
and the tools at `C:\Users\Tyler\.espressif-c61`, including the installed
Python 3.11 environment. It changes only the current PowerShell session.
These defaults use your Windows user profile; override `-IdfPath` and
`-ToolsPath` if you put the SDK elsewhere. On another computer, install the SDK
and its tools first; they are not vendored into this repository.

Connect a USB data cable to the board's **native ESP32 USB port**. Console output
is configured for USB Serial/JTAG. Find the port:

```powershell
python -m serial.tools.list_ports -v
```

Then replace `COM3` below if your board uses another port:

```powershell
idf.py -p COM3 flash monitor
```

Exit the monitor with Ctrl+]. If connecting through the USB-to-UART port instead,
select UART console in `idf.py menuconfig` (Component config > ESP System Settings
> Channel for console output) to see logs through that port.

## Bench check

Validation: the full ESP32-C61 firmware build passed using ESP-IDF 5.5.5,
including bootloader, application image, and partition size checks. Flashing
and the following physical breadboard checks have not been performed yet.

- After reset, the screen stays off and the monitor prints `Ready. Press SCAN...`.
- A brief button press shows the two-line greeting and one `SCAN pressed` log.
- Holding the button does not continually generate new presses.
- Releasing and pressing again produces one new press log.
- If held during startup, release and press it again to trigger the greeting.
- If the display is missing, the monitor reports the address and wiring checks.
  Reconnect the display with power removed, then reset the board.

This milestone does not receive drone broadcasts yet. The later SCAN behavior
will scan for Remote ID and select the nearest aircraft using a method still to
be defined.

## References

- [ESP32-C61 support status](https://developer.espressif.com/hardware/esp32c61/)
- [ESP-IDF's SSD1306 example](https://github.com/espressif/esp-idf/tree/v5.5.5/examples/peripherals/lcd/i2c_oled)
- [Breadboard wiring](../hardware/wiring/breadboard.md)

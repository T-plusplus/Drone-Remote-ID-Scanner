# Code

Reserved for the breadboard proof-of-concept firmware, followed by the final
board firmware. No application code or build framework has been added yet.

Initial pin plan: SDA = GPIO0; SCL = GPIO1; button = GPIO2, active low.
The button is SCAN: one press starts scanning, then the OLED displays the
nearest aircraft's Remote ID. The proximity-selection method is still to be
defined; no firmware behavior has been implemented yet.
When firmware work begins, explicitly configure these pins instead of using
default pin assignments from examples for other ESP32 variants.

#pragma once

#include "esp_lcd_panel_ops.h"

esp_lcd_panel_handle_t oled_init(void);
void oled_show(esp_lcd_panel_handle_t panel, const char *line0, const char *line1,
                const char *line2, const char *line3);

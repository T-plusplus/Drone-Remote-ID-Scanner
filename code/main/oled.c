#include "oled.h"
#include <string.h>
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "fonts/font8x8_basic.h"

#define OLED_SDA GPIO_NUM_0
#define OLED_SCL GPIO_NUM_1
#define OLED_WIDTH 128
#define OLED_HEIGHT 32
static const char *TAG = "oled";
static uint8_t framebuffer[OLED_WIDTH * OLED_HEIGHT / 8];

esp_lcd_panel_handle_t oled_init(void)
{
    i2c_master_bus_handle_t bus;
    const i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0, .sda_io_num = OLED_SDA, .scl_io_num = OLED_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT, .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = false,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus));
    vTaskDelay(pdMS_TO_TICKS(100));
    uint8_t address = 0;
    for (uint8_t candidate = 0x3c; candidate <= 0x3d; ++candidate) {
        if (i2c_master_probe(bus, candidate, 100) == ESP_OK) {
            address = candidate;
            break;
        }
    }
    if (!address) {
        ESP_LOGE(TAG, "OLED not found. Check VCC=3V3, GND, SDA=0, SCL=1 and pull-ups.");
        ESP_ERROR_CHECK(i2c_del_master_bus(bus));
        return NULL;
    }
    ESP_LOGI(TAG, "OLED found at 0x%02X", address);
    esp_lcd_panel_io_handle_t io;
    const esp_lcd_panel_io_i2c_config_t io_config = {
        .dev_addr = address, .scl_speed_hz = 100000, .control_phase_bytes = 1,
        .dc_bit_offset = 6, .lcd_cmd_bits = 8, .lcd_param_bits = 8, .transaction_timeout_ms = 100,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(bus, &io_config, &io));
    esp_lcd_panel_ssd1306_config_t oled_config = {.height = OLED_HEIGHT};
    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = -1, .bits_per_pixel = 1, .vendor_config = &oled_config,
    };
    esp_lcd_panel_handle_t panel;
    ESP_ERROR_CHECK(esp_lcd_new_panel_ssd1306(io, &panel_config, &panel));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel, false));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel, true, true));
    return panel;
}

void oled_show(esp_lcd_panel_handle_t panel, const char *line0, const char *line1,
                const char *line2, const char *line3)
{
    const char *lines[] = {line0, line1, line2, line3};
    memset(framebuffer, 0, sizeof(framebuffer));
    for (unsigned line = 0; line < 4; ++line) {
        if (!lines[line]) {
            continue;
        }
        for (unsigned column = 0; column < 16 && lines[line][column]; ++column) {
            unsigned char ch = (unsigned char)lines[line][column];
            if (ch >= 128) {
                ch = '?';
            }
            for (unsigned y = 0; y < 8; ++y) {
                for (unsigned x = 0; x < 8; ++x) {
                    if (font8x8_basic[ch][y] & (1u << x)) {
                        framebuffer[line * OLED_WIDTH + column * 8 + x] |= 1u << y;
                    }
                }
            }
        }
    }
    ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panel, 0, 0, OLED_WIDTH, OLED_HEIGHT, framebuffer));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel, true));
}

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// GPIO numbers match the board labels in hardware/wiring/breadboard.md.
#define OLED_SDA GPIO_NUM_0
#define OLED_SCL GPIO_NUM_1
#define SCAN_BUTTON GPIO_NUM_2
#define OLED_WIDTH 128
#define OLED_HEIGHT 32
#define BUTTON_DEBOUNCE_MS 30

static const char *TAG = "hello_oled";
static uint8_t framebuffer[OLED_WIDTH * OLED_HEIGHT / 8];

// Only the ten glyphs needed for "Hello, world!", stored as 5 x 7 columns.
static const char glyph_chars[] = "Helo, wrd!";
static const uint8_t glyphs[][5] = {
    {0x7f, 0x08, 0x08, 0x08, 0x7f}, // H
    {0x38, 0x54, 0x54, 0x54, 0x18}, // e
    {0x00, 0x41, 0x7f, 0x40, 0x00}, // l
    {0x38, 0x44, 0x44, 0x44, 0x38}, // o
    {0x00, 0x50, 0x30, 0x00, 0x00}, // ,
    {0x00, 0x00, 0x00, 0x00, 0x00}, // space
    {0x3c, 0x40, 0x30, 0x40, 0x3c}, // w
    {0x7c, 0x08, 0x04, 0x04, 0x08}, // r
    {0x38, 0x44, 0x44, 0x48, 0x7f}, // d
    {0x00, 0x00, 0x5f, 0x00, 0x00}, // !
};

static void draw_text(const char *text, int y)
{
    const int scale = 2;
    int x = (OLED_WIDTH - ((int)strlen(text) * 6 - 1) * scale) / 2;
    for (; *text; ++text, x += 6 * scale) {
        const char *glyph = strchr(glyph_chars, *text);
        if (!glyph) {
            continue;
        }
        for (int column = 0; column < 5; ++column) {
            for (int row = 0; row < 7; ++row) {
                if (!(glyphs[glyph - glyph_chars][column] & (1u << row))) {
                    continue;
                }
                for (int dy = 0; dy < scale; ++dy) {
                    for (int dx = 0; dx < scale; ++dx) {
                        int px = x + column * scale + dx;
                        int py = y + row * scale + dy;
                        if (px >= 0 && px < OLED_WIDTH && py >= 0 && py < OLED_HEIGHT) {
                            framebuffer[(py / 8) * OLED_WIDTH + px] |= 1u << (py % 8);
                        }
                    }
                }
            }
        }
    }
}

static esp_lcd_panel_handle_t init_oled(void)
{
    i2c_master_bus_handle_t bus;
    const i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = OLED_SDA,
        .scl_io_num = OLED_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        // The MakerFocus breakout is expected to provide SDA/SCL pull-ups.
        .flags.enable_internal_pullup = false,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus));
    vTaskDelay(pdMS_TO_TICKS(100)); // Let the breakout settle after USB power-up.

    // SSD1306 breakouts normally use one of these seven-bit addresses.
    uint8_t address = 0;
    for (uint8_t candidate = 0x3c; candidate <= 0x3d; ++candidate) {
        if (i2c_master_probe(bus, candidate, 100) == ESP_OK) {
            address = candidate;
            break;
        }
    }
    if (!address) {
        ESP_LOGE(TAG, "OLED not found at 0x3C/0x3D. Check VCC=3V3, GND, SDA=0, SCL=1 and pull-ups.");
        ESP_ERROR_CHECK(i2c_del_master_bus(bus));
        return NULL;
    }
    ESP_LOGI(TAG, "OLED found at 0x%02X", address);

    esp_lcd_panel_io_handle_t io;
    const esp_lcd_panel_io_i2c_config_t io_config = {
        .dev_addr = address,
        .scl_speed_hz = 100000,
        .control_phase_bytes = 1,
        .dc_bit_offset = 6,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .transaction_timeout_ms = 100,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(bus, &io_config, &io));

    esp_lcd_panel_ssd1306_config_t oled_config = {.height = OLED_HEIGHT};
    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = -1, // Four-pin breakout has no separate reset input.
        .bits_per_pixel = 1,
        .vendor_config = &oled_config,
    };
    esp_lcd_panel_handle_t panel;
    ESP_ERROR_CHECK(esp_lcd_new_panel_ssd1306(io, &panel_config, &panel));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel, false));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel, true, true));

    // Fill display RAM while the screen is off, so the button reveals the greeting.
    draw_text("Hello,", 1);
    draw_text("world!", 17);
    ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panel, 0, 0, OLED_WIDTH, OLED_HEIGHT, framebuffer));
    return panel;
}

void app_main(void)
{
    const gpio_config_t button_config = {
        .pin_bit_mask = 1ULL << SCAN_BUTTON,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE, // Also works if the external 10 kOhm is omitted.
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&button_config));
    esp_lcd_panel_handle_t panel = init_oled();
    if (!panel) {
        return;
    }

    ESP_LOGI(TAG, "Ready. Press SCAN (GPIO2 to GND) to show Hello, world!");
    int last_reading = gpio_get_level(SCAN_BUTTON);
    int stable_level = last_reading;
    TickType_t last_change = xTaskGetTickCount();

    while (true) {
        int reading = gpio_get_level(SCAN_BUTTON);
        TickType_t now = xTaskGetTickCount();
        if (reading != last_reading) {
            last_reading = reading;
            last_change = now;
        }
        if (reading != stable_level && now - last_change >= pdMS_TO_TICKS(BUTTON_DEBOUNCE_MS)) {
            stable_level = reading;
            if (stable_level == 0) {
                ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel, true));
                ESP_LOGI(TAG, "SCAN pressed: Hello, world!");
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

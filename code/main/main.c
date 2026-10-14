#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "oled.h"
#include "rid_scanner.h"

#define SCAN_BUTTON GPIO_NUM_2
#define BUTTON_DEBOUNCE_MS 30

static const char *TAG = "remote_id";
static esp_lcd_panel_handle_t display;

static void scan_progress(unsigned remaining)
{
    char countdown[17];
    snprintf(countdown, sizeof(countdown), "%u seconds left", remaining);
    oled_show(display, "Scanning...", countdown, "", "Please wait");
}

static void show_result(const rid_result_t *result)
{
    if (!result->found) {
        oled_show(display, "No drones found", "", "", "SCAN to rescan");
        return;
    }
    char header[17];
    snprintf(header, sizeof(header), "RID %ddBm %s", result->rssi,
             result->transport == RID_TRANSPORT_BLE ? "BLE" : "WiFi");
    char lines[3][17] = {{0}};
    size_t length = strlen(result->id.text);
    for (size_t row = 0; row < 3; ++row) {
        size_t offset = row * 16;
        if (offset < length) {
            size_t amount = length - offset < 16 ? length - offset : 16;
            memcpy(lines[row], result->id.text + offset, amount);
        }
    }
    if (length <= 32) {
        memcpy(lines[2], "SCAN to rescan", sizeof("SCAN to rescan"));
    }
    oled_show(display, header, lines[0], lines[1], lines[2]);
}

void app_main(void)
{
    const gpio_config_t button_config = {
        .pin_bit_mask = 1ULL << SCAN_BUTTON, .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE, .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&button_config));
    display = oled_init();
    if (!display) {
        return;
    }
    oled_show(display, "Starting radios", "", "", "Please wait");
    esp_err_t err = rid_scanner_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Radio initialization failed: %s", esp_err_to_name(err));
        oled_show(display, "Radio error", "Check USB log", "", "Reset to retry");
        return;
    }
    oled_show(display, "Remote ID ready", "Press SCAN", "", "6 second scan");
    ESP_LOGI(TAG, "Ready. Press SCAN for a six-second Remote ID scan; strongest RSSI wins.");
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
                rid_result_t result;
                scan_progress(RID_SCAN_MS / 1000);
                err = rid_scanner_scan(&result, scan_progress);
                if (err == ESP_OK) {
                    show_result(&result);
                } else {
                    ESP_LOGE(TAG, "Scan failed: %s", esp_err_to_name(err));
                    oled_show(display, "Scan error", "Check USB log", "", "SCAN to retry");
                }
                // Ignore presses during a scan. Require release before retriggering.
                last_reading = gpio_get_level(SCAN_BUTTON);
                stable_level = last_reading;
                last_change = xTaskGetTickCount();
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

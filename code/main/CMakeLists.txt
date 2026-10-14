idf_component_register(SRCS "main.c" "oled.c" "rid_protocol.c" "rid_scanner.c"
                       INCLUDE_DIRS "."
                       REQUIRES esp_driver_gpio esp_driver_i2c esp_lcd esp_wifi esp_event
                                esp_netif esp_timer nvs_flash bt opendroneid)

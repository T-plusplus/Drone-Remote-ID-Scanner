#pragma once

#include "esp_err.h"
#include "rid_protocol.h"

#define RID_BLE_SCAN_MS 3000
#define RID_WIFI_SCAN_MS 3000
#define RID_SCAN_MS (RID_BLE_SCAN_MS + RID_WIFI_SCAN_MS)

typedef void (*rid_progress_fn)(unsigned seconds_remaining);

esp_err_t rid_scanner_init(void);
esp_err_t rid_scanner_scan(rid_result_t *result, rid_progress_fn progress);

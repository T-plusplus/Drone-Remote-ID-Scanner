#include "rid_scanner.h"

#include <string.h>
#include "esp_event.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"

static const char *TAG = "rid_radio";
#define BLE_READY BIT0
#define REPORT_QUEUE_LENGTH 64

typedef struct {
    uint32_t generation;
    rid_id_t id;
    int rssi;
    rid_transport_t transport;
} report_t;

typedef enum { PHASE_IDLE, PHASE_BLE, PHASE_WIFI } phase_t;
static QueueHandle_t reports;
static EventGroupHandle_t ble_events;
static portMUX_TYPE state_lock = portMUX_INITIALIZER_UNLOCKED;
static phase_t phase;
static uint32_t generation;
static bool scan_failed;
static rid_ad_assembler_t ad_assembler; // Accessed only by the NimBLE host task.

static uint32_t capture_generation(phase_t expected_phase)
{
    portENTER_CRITICAL(&state_lock);
    uint32_t value = phase == expected_phase ? generation : 0;
    portEXIT_CRITICAL(&state_lock);
    return value;
}

static void set_phase(phase_t next_phase)
{
    portENTER_CRITICAL(&state_lock);
    phase = next_phase;
    portEXIT_CRITICAL(&state_lock);
}

static void send_ids(const rid_id_t *ids, size_t count, uint32_t scan_generation,
                     int rssi, rid_transport_t transport)
{
    if (rssi < -127 || rssi > 20) {
        return;
    }
    for (size_t i = 0; i < count; ++i) {
        report_t report = {
            .generation = scan_generation, .id = ids[i], .rssi = rssi, .transport = transport,
        };
        // Radio callbacks never wait on the UI, do I2C transfers, or print packet logs.
        if (xQueueSend(reports, &report, 0) != pdTRUE) {
            portENTER_CRITICAL(&state_lock);
            if (phase != PHASE_IDLE && generation == scan_generation) {
                scan_failed = true;
            }
            portEXIT_CRITICAL(&state_lock);
        }
    }
}

static int ble_event(struct ble_gap_event *event, void *arg)
{
    (void)arg;
    uint32_t scan_generation = capture_generation(PHASE_BLE);
    if (!scan_generation) {
        return 0;
    }
    rid_id_t ids[RID_IDS_PER_PACKET];
    if (event->type == BLE_GAP_EVENT_EXT_DISC) {
        const struct ble_gap_ext_disc_desc *desc = &event->ext_disc;
        uint8_t data[RID_AD_MAX_SIZE];
        size_t length = rid_ad_assemble(&ad_assembler, desc->addr.val, desc->sid,
                                        scan_generation, desc->data_status,
                                        desc->data, desc->length_data, data);
        size_t count = rid_parse_ble(data, length, ids);
        send_ids(ids, count, scan_generation, desc->rssi, RID_TRANSPORT_BLE);
    } else if (event->type == BLE_GAP_EVENT_DISC) {
        size_t count = rid_parse_ble(event->disc.data, event->disc.length_data, ids);
        send_ids(ids, count, scan_generation, event->disc.rssi, RID_TRANSPORT_BLE);
    } else if (event->type == BLE_GAP_EVENT_DISC_COMPLETE) {
        // We request an indefinite scan and explicitly stop it after the BLE phase.
        portENTER_CRITICAL(&state_lock);
        if (phase == PHASE_BLE) {
            scan_failed = true;
        }
        portEXIT_CRITICAL(&state_lock);
    }
    return 0;
}

static void ble_sync(void)
{
    int rc = ble_hs_util_ensure_addr(0);
    if (rc == 0) {
        xEventGroupSetBits(ble_events, BLE_READY);
    } else {
        ESP_LOGE(TAG, "BLE address setup failed: %d", rc);
    }
}

static void ble_reset(int reason)
{
    xEventGroupClearBits(ble_events, BLE_READY);
    portENTER_CRITICAL(&state_lock);
    if (phase != PHASE_IDLE) {
        scan_failed = true;
    }
    portEXIT_CRITICAL(&state_lock);
    ESP_LOGE(TAG, "BLE host reset: %d", reason);
}

static void ble_host_task(void *arg)
{
    (void)arg;
    nimble_port_run();
    nimble_port_freertos_deinit();
}

static void wifi_packet(void *buffer, wifi_promiscuous_pkt_type_t type)
{
    uint32_t scan_generation = capture_generation(PHASE_WIFI);
    if (!scan_generation || type != WIFI_PKT_MGMT || !buffer) {
        return;
    }
    const wifi_promiscuous_pkt_t *packet = buffer;
    // ESP-IDF includes four FCS bytes in sig_len. Do not treat them as IEs.
    if (packet->rx_ctrl.sig_len < 4 || packet->rx_ctrl.rx_state != 0) {
        return;
    }
    rid_id_t ids[RID_IDS_PER_PACKET];
    size_t count = rid_parse_wifi(packet->payload, packet->rx_ctrl.sig_len - 4, ids);
    send_ids(ids, count, scan_generation, packet->rx_ctrl.rssi, RID_TRANSPORT_WIFI);
}

esp_err_t rid_scanner_init(void)
{
    reports = xQueueCreate(REPORT_QUEUE_LENGTH, sizeof(report_t));
    ble_events = xEventGroupCreate();
    if (!reports || !ble_events) {
        return ESP_ERR_NO_MEM;
    }
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        err = nvs_flash_erase();
        if (err == ESP_OK) {
            err = nvs_flash_init();
        }
    }
    if (err != ESP_OK) {
        return err;
    }
    ESP_RETURN_ON_ERROR(esp_netif_init(), TAG, "Network interface initialization failed");
    ESP_RETURN_ON_ERROR(esp_event_loop_create_default(), TAG, "Event loop initialization failed");
    wifi_init_config_t wifi_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(esp_wifi_init(&wifi_config), TAG, "Wi-Fi initialization failed");
    ESP_RETURN_ON_ERROR(esp_wifi_set_storage(WIFI_STORAGE_RAM), TAG, "Wi-Fi storage setup failed");
    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_NULL), TAG, "Wi-Fi passive mode setup failed");
    ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "Wi-Fi start failed");
    ESP_RETURN_ON_ERROR(esp_wifi_set_ps(WIFI_PS_NONE), TAG, "Wi-Fi power mode setup failed");
    wifi_promiscuous_filter_t filter = {.filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT};
    ESP_RETURN_ON_ERROR(esp_wifi_set_promiscuous_filter(&filter), TAG, "Wi-Fi filter setup failed");
    ESP_RETURN_ON_ERROR(esp_wifi_set_promiscuous_rx_cb(wifi_packet), TAG, "Wi-Fi callback setup failed");

    err = nimble_port_init();
    if (err != ESP_OK) {
        return err;
    }
    ble_hs_cfg.sync_cb = ble_sync;
    ble_hs_cfg.reset_cb = ble_reset;
    nimble_port_freertos_init(ble_host_task);
    EventBits_t ready = xEventGroupWaitBits(ble_events, BLE_READY, pdFALSE, pdTRUE, pdMS_TO_TICKS(5000));
    return ready & BLE_READY ? ESP_OK : ESP_ERR_TIMEOUT;
}

static void consume_reports(rid_result_t *result, uint32_t scan_generation)
{
    report_t report;
    while (xQueueReceive(reports, &report, 0) == pdTRUE) {
        if (report.generation == scan_generation) {
            rid_consider(result, &report.id, report.rssi, report.transport);
        }
    }
}

static void listen_until(int64_t deadline, int64_t scan_started, uint32_t scan_generation,
                         rid_result_t *result, rid_progress_fn progress, unsigned *last_remaining)
{
    while (esp_timer_get_time() < deadline) {
        consume_reports(result, scan_generation);
        int64_t elapsed_ms = (esp_timer_get_time() - scan_started) / 1000;
        unsigned remaining = elapsed_ms < RID_SCAN_MS ? (RID_SCAN_MS - elapsed_ms + 999) / 1000 : 0;
        if (progress && remaining != *last_remaining) {
            progress(remaining);
            *last_remaining = remaining;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

esp_err_t rid_scanner_scan(rid_result_t *result, rid_progress_fn progress)
{
    if (!result || !reports || !(xEventGroupGetBits(ble_events) & BLE_READY)) {
        return ESP_ERR_INVALID_STATE;
    }
    memset(result, 0, sizeof(*result));
    xQueueReset(reports);
    portENTER_CRITICAL(&state_lock);
    ++generation;
    if (!generation) {
        ++generation;
    }
    uint32_t scan_generation = generation;
    scan_failed = false;
    phase = PHASE_BLE;
    portEXIT_CRITICAL(&state_lock);

    ESP_LOGI(TAG, "SCAN: 3 seconds BLE (1M + coded PHY), then 3 seconds Wi-Fi");
    const struct ble_gap_ext_disc_params phy = {.itvl = 80, .window = 40, .passive = 1};
    int rc = ble_gap_ext_disc(BLE_OWN_ADDR_PUBLIC, 0, 0, 0, 0, 0, &phy, &phy, ble_event, NULL);
    if (rc != 0) {
        set_phase(PHASE_IDLE);
        ESP_LOGE(TAG, "BLE scan could not start: %d", rc);
        return ESP_FAIL;
    }
    int64_t started = esp_timer_get_time();
    unsigned last_remaining = RID_SCAN_MS / 1000;
    listen_until(started + RID_BLE_SCAN_MS * 1000LL, started, scan_generation, result, progress, &last_remaining);
    set_phase(PHASE_IDLE);
    rc = ble_gap_disc_cancel();
    if (rc != 0 && rc != BLE_HS_EALREADY) {
        ESP_LOGE(TAG, "BLE scan could not stop: %d", rc);
        return ESP_FAIL;
    }
    consume_reports(result, scan_generation);

    // Channel 6 gets a full second for NAN and common RID beacons. The other
    // channels (1-11) get 200 ms each. No Wi-Fi or BLE scan requests are sent.
    static const uint8_t channels[] = {6, 1, 2, 3, 4, 5, 7, 8, 9, 10, 11};
    int64_t wifi_started = esp_timer_get_time();
    esp_err_t err = ESP_OK;
    for (size_t i = 0; i < sizeof(channels); ++i) {
        err = esp_wifi_set_channel(channels[i], WIFI_SECOND_CHAN_NONE);
        if (err != ESP_OK) {
            break;
        }
        if (i == 0) {
            set_phase(PHASE_WIFI);
            err = esp_wifi_set_promiscuous(true);
            if (err != ESP_OK) {
                break;
            }
        }
        int64_t channel_deadline = wifi_started + (1000 + (int64_t)i * 200) * 1000;
        listen_until(channel_deadline, started, scan_generation, result, progress, &last_remaining);
    }
    set_phase(PHASE_IDLE);
    esp_err_t stop_err = esp_wifi_set_promiscuous(false);
    consume_reports(result, scan_generation);
    portENTER_CRITICAL(&state_lock);
    bool failed = scan_failed;
    portEXIT_CRITICAL(&state_lock);
    if (err != ESP_OK || stop_err != ESP_OK || failed) {
        ESP_LOGE(TAG, "Scan incomplete: Wi-Fi=%s stop=%s reset/queue error=%d",
                 esp_err_to_name(err), esp_err_to_name(stop_err), failed);
        return ESP_FAIL;
    }
    if (result->found) {
        ESP_LOGI(TAG, "Strongest RID: %s type=%u RSSI=%d dBm via %s (%u Basic ID reports)",
                 result->id.text, result->id.type, result->rssi,
                 result->transport == RID_TRANSPORT_BLE ? "BLE" : "Wi-Fi", result->reports);
    } else {
        ESP_LOGI(TAG, "No drones found: no valid Basic ID received during this scan");
    }
    return ESP_OK;
}

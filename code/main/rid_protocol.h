#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define RID_ID_TEXT_SIZE 41
#define RID_IDS_PER_PACKET 2
#define RID_AD_MAX_SIZE 512
#define RID_AD_SLOTS 4

typedef struct {
    uint8_t type;
    char text[RID_ID_TEXT_SIZE];
} rid_id_t;

// Input is advertising data, or an 802.11 management frame without its FCS.
size_t rid_parse_ble(const uint8_t *data, size_t len, rid_id_t out[RID_IDS_PER_PACKET]);
size_t rid_parse_wifi(const uint8_t *data, size_t len, rid_id_t out[RID_IDS_PER_PACKET]);

typedef struct {
    bool used;
    bool discarded;
    uint8_t address[6];
    uint8_t sid;
    uint32_t generation;
    size_t length;
    uint8_t data[RID_AD_MAX_SIZE];
} rid_ad_slot_t;

typedef struct {
    rid_ad_slot_t slots[RID_AD_SLOTS];
    unsigned next_slot;
} rid_ad_assembler_t;

// BLE HCI status: 0 = complete, 1 = more fragments, 2 = truncated.
size_t rid_ad_assemble(rid_ad_assembler_t *assembler, const uint8_t address[6],
                       uint8_t sid, uint32_t generation, uint8_t status,
                       const uint8_t *data, size_t len, uint8_t out[RID_AD_MAX_SIZE]);

typedef enum { RID_TRANSPORT_BLE, RID_TRANSPORT_WIFI } rid_transport_t;

typedef struct {
    bool found;
    rid_id_t id;
    int rssi;
    rid_transport_t transport;
    unsigned reports;
} rid_result_t;

void rid_consider(rid_result_t *result, const rid_id_t *id, int rssi,
                  rid_transport_t transport);

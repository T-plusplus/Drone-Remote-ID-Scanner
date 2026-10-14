#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "opendroneid.h"
#include "rid_protocol.h"

static void make_basic(uint8_t out[25], const char *serial)
{
    ODID_BasicID_data data;
    odid_initBasicIDData(&data);
    data.IDType = ODID_IDTYPE_SERIAL_NUMBER;
    data.UAType = ODID_UATYPE_HELICOPTER_OR_MULTIROTOR;
    memcpy(data.UASID, serial, strlen(serial));
    ODID_BasicID_encoded encoded;
    assert(encodeBasicIDMessage(&encoded, &data) == ODID_SUCCESS);
    memcpy(out, &encoded, 25);
}

static size_t ble_record(uint8_t *out, const uint8_t *payload, size_t length)
{
    assert(length <= 250);
    out[0] = (uint8_t)(5 + length);
    out[1] = 0x16;
    out[2] = 0xfa;
    out[3] = 0xff;
    out[4] = 0x0d;
    out[5] = 7; // Transport counter, not part of the Basic ID message.
    memcpy(out + 6, payload, length);
    return 6 + length;
}

static void test_ble_and_packs(void)
{
    uint8_t basic[25], record[256];
    rid_id_t ids[2];
    const char serial[] = "1581F123456789ABCDEF";
    assert(strlen(serial) == 20);
    make_basic(basic, serial);
    size_t length = ble_record(record, basic, 25);
    assert(rid_parse_ble(record, length, ids) == 1);
    assert(strcmp(ids[0].text, serial) == 0);
    for (size_t truncated = 0; truncated < length; ++truncated) {
        assert(rid_parse_ble(record, truncated, ids) == 0);
    }
    record[2] = 0xfb;
    assert(rid_parse_ble(record, length, ids) == 0);
    record[2] = 0xfa;
    record[4] = 0x0c;
    assert(rid_parse_ble(record, length, ids) == 0);
    record[4] = 0x0d;
    record[6] = 0x03; // Unknown protocol version.
    assert(rid_parse_ble(record, length, ids) == 0);

    uint8_t pack[53] = {0xf2, 25, 2};
    memcpy(pack + 3, basic, 25);
    make_basic(pack + 28, "SECOND-DRONE");
    length = ble_record(record, pack, sizeof(pack));
    assert(rid_parse_ble(record, length, ids) == 2);
    assert(strcmp(ids[1].text, "SECOND-DRONE") == 0);
    for (size_t truncated = 0; truncated < length; ++truncated) {
        assert(rid_parse_ble(record, truncated, ids) == 0);
    }
    record[8] = 9; // Count exceeds actual payload.
    assert(rid_parse_ble(record, length, ids) == 0);
    record[8] = 2;
    record[9] = 0xf2; // Nested pack must not be interpreted as Basic ID.
    assert(rid_parse_ble(record, length, ids) == 0);

    memset(basic, 0, 25); // ID type NONE.
    length = ble_record(record, basic, 25);
    assert(rid_parse_ble(record, length, ids) == 0);
    make_basic(basic, "   ");
    length = ble_record(record, basic, 25);
    assert(rid_parse_ble(record, length, ids) == 0);
    make_basic(basic, "GOOD");
    basic[2] = 1; // Control character in the serial.
    length = ble_record(record, basic, 25);
    assert(rid_parse_ble(record, length, ids) == 0);
    memset(basic + 2, 0, 20);
    basic[1] = 0x32; // UUID Basic ID.
    basic[2] = 0xab;
    basic[17] = 0xcd;
    length = ble_record(record, basic, 25);
    assert(rid_parse_ble(record, length, ids) == 1);
    assert(strcmp(ids[0].text, "AB0000000000000000000000000000CD") == 0);
}

static void test_wifi(void)
{
    uint8_t pack[28] = {0xf2, 25, 1};
    make_basic(pack + 3, "WIFI-DRONE");
    uint8_t beacon[128] = {0x80};
    // 24-byte management header + 12-byte beacon fields + empty SSID IE.
    size_t offset = 38;
    const uint8_t vendor[] = {0xdd, 33, 0xfa, 0x0b, 0xbc, 0x0d, 7};
    memcpy(beacon + offset, vendor, sizeof(vendor));
    memcpy(beacon + offset + sizeof(vendor), pack, sizeof(pack));
    size_t length = offset + sizeof(vendor) + sizeof(pack);
    rid_id_t ids[2];
    assert(rid_parse_wifi(beacon, length, ids) == 1);
    assert(strcmp(ids[0].text, "WIFI-DRONE") == 0);
    for (size_t truncated = 0; truncated < length; ++truncated) {
        assert(rid_parse_wifi(beacon, truncated, ids) == 0);
    }
    beacon[43] = 0x01; // Another vendor protocol is not Remote ID.
    assert(rid_parse_wifi(beacon, length, ids) == 0);
    beacon[43] = 0x0d;
    beacon[1] = 0x40; // Encrypted frame.
    assert(rid_parse_wifi(beacon, length, ids) == 0);

    uint8_t nan[128] = {0xd0};
    const uint8_t nan_header[] = {0x04, 0x09, 0x50, 0x6f, 0x9a, 0x13};
    memcpy(nan + 24, nan_header, sizeof(nan_header));
    // An unrelated NAN attribute before the Remote ID service descriptor.
    nan[30] = 1; nan[31] = 2; nan[32] = 0;
    const uint8_t descriptor[] = {
        3, 39, 0, 0x88, 0x69, 0x19, 0x9d, 0x92, 0x09, 1, 0, 0x10, 29, 7,
    };
    memcpy(nan + 35, descriptor, sizeof(descriptor));
    memcpy(nan + 35 + sizeof(descriptor), pack, sizeof(pack));
    length = 35 + sizeof(descriptor) + sizeof(pack);
    assert(rid_parse_wifi(nan, length, ids) == 1);
    for (size_t truncated = 0; truncated < length; ++truncated) {
        assert(rid_parse_wifi(nan, truncated, ids) == 0);
    }
    nan[38] ^= 1; // Wrong service ID.
    assert(rid_parse_wifi(nan, length, ids) == 0);
    nan[38] ^= 1;
    nan[47] = 30; // Inconsistent service-info length.
    assert(rid_parse_wifi(nan, length, ids) == 0);
}

static void test_fragments(void)
{
    rid_ad_assembler_t assembler = {0};
    uint8_t basic[25], record[64], output[RID_AD_MAX_SIZE];
    const uint8_t address[6] = {1, 2, 3, 4, 5, 6};
    const uint8_t other[6] = {7, 8, 9, 10, 11, 12};
    make_basic(basic, "FRAGMENT-DRONE");
    size_t length = ble_record(record, basic, 25);
    assert(rid_ad_assemble(&assembler, address, 1, 1, 1, record, 12, output) == 0);
    assert(rid_ad_assemble(&assembler, other, 1, 1, 1, record, 4, output) == 0);
    assert(rid_ad_assemble(&assembler, address, 1, 1, 0, record + 12, length - 12, output) == length);
    rid_id_t ids[2];
    assert(rid_parse_ble(output, length, ids) == 1);
    assert(strcmp(ids[0].text, "FRAGMENT-DRONE") == 0);
    assert(rid_ad_assemble(&assembler, address, 1, 1, 1, record, 12, output) == 0);
    assert(rid_ad_assemble(&assembler, address, 1, 1, 2, record + 12, length - 12, output) == 0);
    // A new scan cannot combine an old fragment with a new report.
    assert(rid_ad_assemble(&assembler, address, 1, 1, 1, record, 12, output) == 0);
    size_t size = rid_ad_assemble(&assembler, address, 1, 2, 0, record + 12, length - 12, output);
    assert(rid_parse_ble(output, size, ids) == 0);
    uint8_t oversized[RID_AD_MAX_SIZE] = {0};
    assert(rid_ad_assemble(&assembler, address, 1, 3, 1, oversized, sizeof(oversized), output) == 0);
    assert(rid_ad_assemble(&assembler, address, 1, 3, 0, record, length, output) == 0);
}

static void test_selection(void)
{
    rid_result_t result = {0};
    rid_id_t first = {.type = 1, .text = "FIRST"};
    rid_id_t second = {.type = 1, .text = "SECOND"};
    rid_consider(&result, &first, -80, RID_TRANSPORT_BLE);
    rid_consider(&result, &second, -45, RID_TRANSPORT_WIFI);
    assert(strcmp(result.id.text, "SECOND") == 0 && result.rssi == -45);
    rid_consider(&result, &first, 127, RID_TRANSPORT_BLE);
    assert(strcmp(result.id.text, "SECOND") == 0 && result.reports == 2);
    rid_consider(&result, &first, -60, RID_TRANSPORT_BLE);
    assert(strcmp(result.id.text, "SECOND") == 0);
    rid_consider(&result, &first, -30, RID_TRANSPORT_BLE);
    assert(strcmp(result.id.text, "FIRST") == 0);
    rid_id_t session = {.type = 4, .text = "SESSION"};
    rid_consider(&result, &session, -30, RID_TRANSPORT_BLE);
    assert(result.id.type == 1); // Prefer serial on equal RSSI.
    memset(&result, 0, sizeof(result));
    assert(!result.found && !result.reports); // A new scan has no stale candidate.
}

static void test_random_inputs(void)
{
    uint32_t random = 0x9e3779b9;
    uint8_t data[512];
    rid_id_t ids[2];
    for (unsigned trial = 0; trial < 20000; ++trial) {
        for (size_t i = 0; i < sizeof(data); ++i) {
            random = random * 1664525u + 1013904223u;
            data[i] = (uint8_t)(random >> 24);
        }
        size_t length = trial % sizeof(data);
        assert(rid_parse_ble(data, length, ids) <= 2);
        assert(rid_parse_wifi(data, length, ids) <= 2);
    }
}

int main(void)
{
    test_ble_and_packs();
    test_wifi();
    test_fragments();
    test_selection();
    test_random_inputs();
    puts("PASS: BLE, message packs, Wi-Fi beacons/NAN, fragments, RSSI selection and 20,000 malformed inputs");
    return 0;
}

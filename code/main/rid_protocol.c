#include "rid_protocol.h"

#include <string.h>
#include "opendroneid.h"

_Static_assert(sizeof(ODID_BasicID_encoded) == ODID_MESSAGE_SIZE, "Basic ID wire layout");

static bool basic_id(const uint8_t *message, rid_id_t *id)
{
    if ((message[0] >> 4) != ODID_MESSAGETYPE_BASIC_ID || (message[0] & 15) > 2) {
        return false;
    }
    ODID_BasicID_encoded encoded;
    ODID_BasicID_data decoded;
    memcpy(&encoded, message, sizeof(encoded));
    if (decodeBasicIDMessage(&decoded, &encoded) != ODID_SUCCESS ||
        decoded.IDType < ODID_IDTYPE_SERIAL_NUMBER ||
        decoded.IDType > ODID_IDTYPE_SPECIFIC_SESSION_ID) {
        return false;
    }
    memset(id, 0, sizeof(*id));
    id->type = (uint8_t)decoded.IDType;
    if (decoded.IDType == ODID_IDTYPE_SERIAL_NUMBER ||
        decoded.IDType == ODID_IDTYPE_CAA_REGISTRATION_ID) {
        bool nonblank = false;
        for (size_t i = 0; i < ODID_ID_SIZE && decoded.UASID[i]; ++i) {
            unsigned char ch = (unsigned char)decoded.UASID[i];
            if (ch < 32 || ch > 126) {
                return false;
            }
            id->text[i] = (char)ch;
            nonblank |= ch != ' ';
        }
        return nonblank;
    }
    // UUIDs are 16 bytes; session IDs retain all 20 bytes, including format ID.
    size_t bytes = decoded.IDType == ODID_IDTYPE_UTM_ASSIGNED_UUID ? 16 : ODID_ID_SIZE;
    static const char hex[] = "0123456789ABCDEF";
    bool nonzero = false;
    for (size_t i = 0; i < bytes; ++i) {
        uint8_t value = (uint8_t)decoded.UASID[i];
        nonzero |= value != 0;
        id->text[2 * i] = hex[value >> 4];
        id->text[2 * i + 1] = hex[value & 15];
    }
    return nonzero;
}

static size_t payload_ids(const uint8_t *data, size_t len, rid_id_t *out, size_t capacity)
{
    if (!data || !len || !capacity || (data[0] & 15) > 2) {
        return 0;
    }
    if ((data[0] >> 4) == ODID_MESSAGETYPE_BASIC_ID) {
        return len == ODID_MESSAGE_SIZE && basic_id(data, out) ? 1 : 0;
    }
    if ((data[0] >> 4) != ODID_MESSAGETYPE_PACKED || len < 3 ||
        data[1] != ODID_MESSAGE_SIZE || data[2] == 0 || data[2] > ODID_PACK_MAX_MESSAGES ||
        len != 3u + (size_t)data[2] * ODID_MESSAGE_SIZE) {
        return 0;
    }
    unsigned types[6] = {0};
    for (size_t i = 0; i < data[2]; ++i) {
        const uint8_t *message = data + 3 + i * ODID_MESSAGE_SIZE;
        unsigned type = message[0] >> 4;
        if (type > ODID_MESSAGETYPE_OPERATOR_ID || (message[0] & 15) > 2) {
            return 0; // Reject nested packs and unsupported protocol versions.
        }
        ++types[type];
    }
    if (types[0] > ODID_BASIC_ID_MAX_MESSAGES || types[1] > 1 || types[3] > 1 ||
        types[4] > 1 || types[5] > 1) {
        return 0;
    }
    size_t count = 0;
    for (size_t i = 0; i < data[2] && count < capacity; ++i) {
        if (basic_id(data + 3 + i * ODID_MESSAGE_SIZE, &out[count])) {
            ++count;
        }
    }
    return count;
}

size_t rid_parse_ble(const uint8_t *data, size_t len, rid_id_t out[RID_IDS_PER_PACKET])
{
    size_t count = 0;
    if (!data || !out) {
        return 0;
    }
    for (size_t offset = 0; offset < len && count < RID_IDS_PER_PACKET;) {
        size_t length = data[offset];
        if (!length || length > len - offset - 1) {
            break;
        }
        const uint8_t *field = data + offset + 1;
        // AD type 0x16, ASTM service UUID 0xFFFA, OpenDroneID app code 0x0D,
        // then the transport message counter and the encoded ODID message(s).
        if (length >= 5 && field[0] == 0x16 && field[1] == 0xfa &&
            field[2] == 0xff && field[3] == 0x0d) {
            count += payload_ids(field + 5, length - 5, out + count, RID_IDS_PER_PACKET - count);
        }
        offset += 1 + length;
    }
    return count;
}

size_t rid_parse_wifi(const uint8_t *data, size_t len, rid_id_t out[RID_IDS_PER_PACKET])
{
    if (!data || !out || len < 24 || (data[0] & 15) ||
        (data[1] & 0x44) || (data[22] & 15)) {
        return 0; // Management frames only; skip encrypted/fragmented frames.
    }
    size_t header = (data[1] & 0x80) ? 28 : 24;
    if (len < header) {
        return 0;
    }
    size_t count = 0;
    if ((data[0] & 0xf0) == 0x80) {
        if (len < header + 12) {
            return 0;
        }
        for (size_t offset = header + 12; offset + 2 <= len && count < RID_IDS_PER_PACKET;) {
            size_t length = data[offset + 1];
            if (length > len - offset - 2) {
                break;
            }
            const uint8_t *field = data + offset + 2;
            // ASTM/ASD-STAN vendor IE: FA:0B:BC, type 0x0D, counter, pack.
            if (data[offset] == 0xdd && length >= 8 && field[0] == 0xfa &&
                field[1] == 0x0b && field[2] == 0xbc && field[3] == 0x0d &&
                (field[5] >> 4) == ODID_MESSAGETYPE_PACKED) {
                count += payload_ids(field + 5, length - 5, out + count, RID_IDS_PER_PACKET - count);
            }
            offset += 2 + length;
        }
    } else if ((data[0] & 0xf0) == 0xd0) {
        static const uint8_t nan_header[] = {0x04, 0x09, 0x50, 0x6f, 0x9a, 0x13};
        static const uint8_t service_id[] = {0x88, 0x69, 0x19, 0x9d, 0x92, 0x09};
        if (len < header + sizeof(nan_header) ||
            memcmp(data + header, nan_header, sizeof(nan_header))) {
            return 0;
        }
        for (size_t offset = header + sizeof(nan_header);
             offset + 3 <= len && count < RID_IDS_PER_PACKET;) {
            size_t length = data[offset + 1] | ((size_t)data[offset + 2] << 8);
            if (length > len - offset - 3) {
                break;
            }
            const uint8_t *field = data + offset + 3;
            if (data[offset] == 3 && length >= 11 && !memcmp(field, service_id, 6) &&
                field[8] == 0x10 && field[9] == length - 10 && field[9] >= 4 &&
                (field[11] >> 4) == ODID_MESSAGETYPE_PACKED) {
                count += payload_ids(field + 11, field[9] - 1, out + count, RID_IDS_PER_PACKET - count);
            }
            offset += 3 + length;
        }
    }
    return count;
}

size_t rid_ad_assemble(rid_ad_assembler_t *assembler, const uint8_t address[6],
                       uint8_t sid, uint32_t generation, uint8_t status,
                       const uint8_t *data, size_t len, uint8_t out[RID_AD_MAX_SIZE])
{
    if (!assembler || !address || !out || (!data && len) || !generation) {
        return 0;
    }
    rid_ad_slot_t *slot = NULL;
    for (unsigned i = 0; i < RID_AD_SLOTS; ++i) {
        rid_ad_slot_t *candidate = &assembler->slots[i];
        if (candidate->used && candidate->generation == generation && candidate->sid == sid &&
            !memcmp(candidate->address, address, 6)) {
            slot = candidate;
            break;
        }
    }
    if (!slot) {
        if (status == 0) {
            if (len > RID_AD_MAX_SIZE) {
                return 0;
            }
            if (len) {
                memcpy(out, data, len);
            }
            return len;
        }
        if (status != 1) {
            return 0;
        }
        slot = &assembler->slots[assembler->next_slot++ % RID_AD_SLOTS];
        memset(slot, 0, sizeof(*slot));
        slot->used = true;
        slot->generation = generation;
        slot->sid = sid;
        memcpy(slot->address, address, 6);
    }
    if (status > 1) {
        slot->used = false;
        return 0;
    }
    if (len > RID_AD_MAX_SIZE - slot->length) {
        slot->discarded = true;
    }
    if (!slot->discarded && len) {
        memcpy(slot->data + slot->length, data, len);
        slot->length += len;
    }
    if (status == 1) {
        return 0;
    }
    size_t completed = slot->discarded ? 0 : slot->length;
    if (completed) {
        memcpy(out, slot->data, completed);
    }
    slot->used = false;
    return completed;
}

void rid_consider(rid_result_t *result, const rid_id_t *id, int rssi,
                  rid_transport_t transport)
{
    if (!result || !id || !id->text[0] || id->type < 1 || id->type > 4 ||
        rssi < -127 || rssi > 20) {
        return; // BLE reports RSSI=127 when unavailable; never rank that as strongest.
    }
    ++result->reports;
    if (!result->found || rssi > result->rssi ||
        (rssi == result->rssi && id->type < result->id.type)) {
        result->found = true;
        result->id = *id;
        result->rssi = rssi;
        result->transport = transport;
    }
}

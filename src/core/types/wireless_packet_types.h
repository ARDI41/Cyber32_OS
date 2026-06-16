#pragma once

#include <stdint.h>

namespace Cyber32 {

static const uint16_t WIRELESS_PACKET_MAGIC = 0x4332;
static const uint8_t WIRELESS_PROTOCOL_VERSION = 1;
static const uint8_t WIRELESS_MAX_PACKET_SIZE = 250;
static const uint8_t WIRELESS_CAPABILITY_ID_SIZE = 24;
static const uint8_t WIRELESS_ERROR_CODE_SIZE = 24;

enum class WirelessPacketType : unsigned char {
    NODE_ANNOUNCE,
    NODE_HEARTBEAT,
    CAPABILITY_VALUE,
    CAPABILITY_STATUS,
    ERROR_REPORT,
    COMMAND_REQUEST,
    COMMAND_ACK,
    COMMAND_RESULT
};

enum class WirelessPayloadType : unsigned char {
    NONE,
    FLOAT,
    INT,
    BOOLEAN
};

struct WirelessPacketHeader {
    uint16_t magic;
    uint8_t protocol_version;
    WirelessPacketType packet_type;
    uint8_t flags;
    uint16_t sequence_id;
    uint32_t node_id;
    uint8_t payload_length;
};

struct WirelessCapabilityValue {
    char capability_id[WIRELESS_CAPABILITY_ID_SIZE];
    WirelessPayloadType payload_type;
    float value_float;
    int32_t value_int;
    char error_code[WIRELESS_ERROR_CODE_SIZE];
};

struct WirelessNodeDiagnostics {
    bool battery_present;
    float battery_level_percent;
    float battery_voltage;
    float signal_quality_percent;
};

}  // namespace Cyber32

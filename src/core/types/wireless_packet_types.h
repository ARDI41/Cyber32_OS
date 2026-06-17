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
    uint16_t checksum;
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

inline uint16_t addWirelessChecksumBytes(
    uint16_t checksum,
    const void* value,
    uint8_t size) {
    const uint8_t* bytes = static_cast<const uint8_t*>(value);
    for (uint8_t i = 0; i < size; ++i) {
        checksum = static_cast<uint16_t>(checksum + bytes[i]);
    }
    return checksum;
}

inline uint16_t addWirelessChecksumUint8(uint16_t checksum, uint8_t value) {
    return static_cast<uint16_t>(checksum + value);
}

inline uint16_t calculateWirelessPacketChecksum(
    const WirelessPacketHeader& header,
    const WirelessCapabilityValue& value,
    const WirelessNodeDiagnostics& diagnostics) {
    uint16_t checksum = 0;

    checksum = addWirelessChecksumBytes(checksum, &header.magic, sizeof(header.magic));
    checksum = addWirelessChecksumUint8(checksum, header.protocol_version);
    checksum = addWirelessChecksumUint8(
        checksum,
        static_cast<uint8_t>(header.packet_type));
    checksum = addWirelessChecksumUint8(checksum, header.flags);
    checksum = addWirelessChecksumBytes(
        checksum,
        &header.sequence_id,
        sizeof(header.sequence_id));
    checksum = addWirelessChecksumBytes(checksum, &header.node_id, sizeof(header.node_id));
    checksum = addWirelessChecksumUint8(checksum, header.payload_length);

    for (uint8_t i = 0; i < WIRELESS_CAPABILITY_ID_SIZE; ++i) {
        checksum = addWirelessChecksumUint8(
            checksum,
            static_cast<uint8_t>(value.capability_id[i]));
    }
    checksum = addWirelessChecksumUint8(
        checksum,
        static_cast<uint8_t>(value.payload_type));
    checksum = addWirelessChecksumBytes(
        checksum,
        &value.value_float,
        sizeof(value.value_float));
    checksum = addWirelessChecksumBytes(checksum, &value.value_int, sizeof(value.value_int));

    for (uint8_t i = 0; i < WIRELESS_ERROR_CODE_SIZE; ++i) {
        checksum = addWirelessChecksumUint8(
            checksum,
            static_cast<uint8_t>(value.error_code[i]));
    }

    const uint8_t battery_present = diagnostics.battery_present ? 1 : 0;
    checksum = addWirelessChecksumUint8(checksum, battery_present);
    checksum = addWirelessChecksumBytes(
        checksum,
        &diagnostics.battery_level_percent,
        sizeof(diagnostics.battery_level_percent));
    checksum = addWirelessChecksumBytes(
        checksum,
        &diagnostics.battery_voltage,
        sizeof(diagnostics.battery_voltage));
    checksum = addWirelessChecksumBytes(
        checksum,
        &diagnostics.signal_quality_percent,
        sizeof(diagnostics.signal_quality_percent));

    return checksum;
}

inline bool wirelessPacketChecksumValid(
    const WirelessPacketHeader& header,
    const WirelessCapabilityValue& value,
    const WirelessNodeDiagnostics& diagnostics) {
    return header.checksum ==
           calculateWirelessPacketChecksum(header, value, diagnostics);
}

}  // namespace Cyber32

#include "sensor_packet_decode_debug.h"

#include <Arduino.h>

#include "../../core/types/wireless_node_allowlist_records.h"

namespace Cyber32 {

namespace {

static const uint16_t SENSOR_FIRMWARE_PACKET_MAGIC = 0xC32B;
static const uint8_t SENSOR_FIRMWARE_PROTOCOL_VERSION = 1;
static const int SENSOR_FIRMWARE_PACKET_SIZE = 60;
static const uint8_t SENSOR_FIRST_BYTES_TO_PRINT = 12;
static const uint16_t SENSOR_CAP_TEMPERATURE_ID = 0x0001;
static const uint8_t SENSOR_PAYLOAD_FLOAT = 1;

static const uint8_t SENSOR_MAGIC_OFFSET = 0;
static const uint8_t SENSOR_PROTOCOL_VERSION_OFFSET = 2;
static const uint8_t SENSOR_PACKET_TYPE_OFFSET = 3;
static const uint8_t SENSOR_PACKET_SIZE_OFFSET = 4;
static const uint8_t SENSOR_NODE_ID_OFFSET = 8;
static const uint8_t SENSOR_CAPABILITY_ID_OFFSET = 20;
static const uint8_t SENSOR_PAYLOAD_SCHEMA_VERSION_OFFSET = 22;
static const uint8_t SENSOR_PAYLOAD_AVAILABLE_OFFSET = 23;
static const uint8_t SENSOR_PAYLOAD_TYPE_OFFSET = 24;
static const uint8_t SENSOR_FLOAT_VALUE_OFFSET = 28;
static const uint8_t SENSOR_SEQUENCE_OFFSET = 40;
static const uint8_t SENSOR_TIMESTAMP_OFFSET = 44;
static const uint8_t SENSOR_BATTERY_MV_OFFSET = 48;
static const uint8_t SENSOR_BATTERY_PERCENT_OFFSET = 50;
static const uint8_t SENSOR_SIGNAL_STRENGTH_OFFSET = 51;
static const uint8_t SENSOR_NODE_STATUS_OFFSET = 52;
static const uint8_t SENSOR_ERROR_CODE_OFFSET = 53;
static const uint8_t SENSOR_PAIRING_FLAGS_OFFSET = 54;
static const uint8_t SENSOR_SECURITY_LEVEL_OFFSET = 55;
static const uint8_t SENSOR_CHECKSUM_OFFSET = 56;

uint16_t readUint16LittleEndian(const uint8_t* data) {
    return static_cast<uint16_t>(
        static_cast<uint16_t>(data[0]) |
        static_cast<uint16_t>(static_cast<uint16_t>(data[1]) << 8));
}

uint32_t readUint32LittleEndian(const uint8_t* data) {
    return static_cast<uint32_t>(
        static_cast<uint32_t>(data[0]) |
        static_cast<uint32_t>(data[1]) << 8 |
        static_cast<uint32_t>(data[2]) << 16 |
        static_cast<uint32_t>(data[3]) << 24);
}

float readFloatBytes(const uint8_t* data) {
    float value = 0.0F;
    uint8_t* value_bytes = reinterpret_cast<uint8_t*>(&value);
    for (uint8_t i = 0; i < sizeof(float); ++i) {
        value_bytes[i] = data[i];
    }
    return value;
}

uint16_t calculateSensorPacketChecksum(const uint8_t* data) {
    uint16_t checksum = 0;
    for (uint8_t i = 0; i < SENSOR_CHECKSUM_OFFSET; ++i) {
        checksum = static_cast<uint16_t>(checksum + data[i]);
    }
    return checksum;
}

void printMacByte(uint8_t value) {
    if (value < 0x10U) {
        Serial.print('0');
    }
    Serial.print(value, HEX);
}

void printFirstBytes(const uint8_t* data, int data_len) {
    Serial.print("Cyber32 Packet First Bytes: ");
    const uint8_t count = data_len < SENSOR_FIRST_BYTES_TO_PRINT
        ? static_cast<uint8_t>(data_len)
        : SENSOR_FIRST_BYTES_TO_PRINT;
    for (uint8_t i = 0; i < count; ++i) {
        if (i > 0U) {
            Serial.print(' ');
        }
        printMacByte(data[i]);
    }
    if (data_len > SENSOR_FIRST_BYTES_TO_PRINT) {
        Serial.print(" ...");
    }
    Serial.println();
}

void printMacAddress(const uint8_t* source_mac) {
    for (uint8_t i = 0; i < WIRELESS_MAC_ADDRESS_SIZE; ++i) {
        if (i > 0U) {
            Serial.print(':');
        }
        if (source_mac == 0) {
            printMacByte(0);
        } else {
            printMacByte(source_mac[i]);
        }
    }
}

void printCapability(uint16_t capability_id) {
    if (capability_id == SENSOR_CAP_TEMPERATURE_ID) {
        Serial.print("CAP_TEMPERATURE");
    } else {
        Serial.print(capability_id);
    }
}

void printPayloadType(uint8_t payload_type) {
    if (payload_type == 0U) {
        Serial.print("NONE");
    } else if (payload_type == SENSOR_PAYLOAD_FLOAT) {
        Serial.print("FLOAT");
    } else if (payload_type == 2U) {
        Serial.print("INT");
    } else if (payload_type == 3U) {
        Serial.print("BOOLEAN");
    } else {
        Serial.print(payload_type);
    }
}

}  // namespace

void printSensorPacketEnvelopeDebug(
    const uint8_t* source_mac,
    const uint8_t* data,
    int data_len) {
    if (data == 0 || data_len != SENSOR_FIRMWARE_PACKET_SIZE) {
        Serial.println("Cyber32 Packet Envelope: INVALID_SIZE");
        return;
    }

    printFirstBytes(data, data_len);

    const uint16_t magic = readUint16LittleEndian(&data[SENSOR_MAGIC_OFFSET]);
    if (magic != SENSOR_FIRMWARE_PACKET_MAGIC) {
        Serial.println("Cyber32 Packet Envelope: INVALID_MAGIC");
        return;
    }

    const uint8_t protocol_version = data[SENSOR_PROTOCOL_VERSION_OFFSET];
    if (protocol_version != SENSOR_FIRMWARE_PROTOCOL_VERSION) {
        Serial.println("Cyber32 Packet Envelope: INVALID_VERSION");
        return;
    }

    const uint16_t packet_size = readUint16LittleEndian(&data[SENSOR_PACKET_SIZE_OFFSET]);
    if (packet_size != SENSOR_FIRMWARE_PACKET_SIZE) {
        Serial.println("Cyber32 Packet Envelope: INVALID_SIZE");
        return;
    }

    Serial.println("Cyber32 Packet Envelope: OK");

    const uint8_t packet_type = data[SENSOR_PACKET_TYPE_OFFSET];
    const uint32_t node_id = readUint32LittleEndian(&data[SENSOR_NODE_ID_OFFSET]);
    const uint16_t capability_id = readUint16LittleEndian(&data[SENSOR_CAPABILITY_ID_OFFSET]);
    const uint8_t payload_schema_version = data[SENSOR_PAYLOAD_SCHEMA_VERSION_OFFSET];
    const uint8_t payload_available = data[SENSOR_PAYLOAD_AVAILABLE_OFFSET];
    const uint8_t payload_type = data[SENSOR_PAYLOAD_TYPE_OFFSET];
    const float value_float = readFloatBytes(&data[SENSOR_FLOAT_VALUE_OFFSET]);
    const uint32_t sequence_id = readUint32LittleEndian(&data[SENSOR_SEQUENCE_OFFSET]);
    const uint32_t timestamp_ms = readUint32LittleEndian(&data[SENSOR_TIMESTAMP_OFFSET]);
    const uint16_t battery_mv = readUint16LittleEndian(&data[SENSOR_BATTERY_MV_OFFSET]);
    const uint8_t battery_percent = data[SENSOR_BATTERY_PERCENT_OFFSET];
    const int8_t signal_strength = static_cast<int8_t>(data[SENSOR_SIGNAL_STRENGTH_OFFSET]);
    const uint8_t node_status = data[SENSOR_NODE_STATUS_OFFSET];
    const uint8_t error_code = data[SENSOR_ERROR_CODE_OFFSET];
    const uint8_t pairing_flags = data[SENSOR_PAIRING_FLAGS_OFFSET];
    const uint8_t security_level = data[SENSOR_SECURITY_LEVEL_OFFSET];
    const uint16_t checksum_field = readUint16LittleEndian(&data[SENSOR_CHECKSUM_OFFSET]);
    const uint16_t calculated_checksum = calculateSensorPacketChecksum(data);

    Serial.println("Cyber32 Sensor Packet:");
    Serial.print("  Source MAC: ");
    printMacAddress(source_mac);
    Serial.println();
    Serial.print("  Length: ");
    Serial.println(data_len);
    Serial.print("  Magic: ");
    Serial.println("OK");
    Serial.print("  Version: ");
    Serial.println("OK");
    Serial.print("  Packet Type: ");
    Serial.println(packet_type);
    Serial.print("  Packet Size: ");
    Serial.println(packet_size);
    Serial.print("  Node ID: ");
    Serial.println(node_id);
    Serial.print("  Sequence ID: ");
    Serial.println(sequence_id);
    Serial.print("  Capability: ");
    printCapability(capability_id);
    Serial.println();
    Serial.print("  Payload Type: ");
    printPayloadType(payload_type);
    Serial.println();
    Serial.print("  Payload Schema Version: ");
    Serial.println(payload_schema_version);
    Serial.print("  Payload Available: ");
    Serial.println(payload_available);
    if (capability_id == SENSOR_CAP_TEMPERATURE_ID &&
        payload_type == SENSOR_PAYLOAD_FLOAT) {
        Serial.print("  Temperature: ");
        Serial.print(value_float);
        Serial.println(" C");
    }
    Serial.print("  Checksum Field: ");
    Serial.println(checksum_field);
    Serial.print("  Calculated Checksum: ");
    Serial.println(calculated_checksum);
    Serial.print("  Checksum: ");
    if (checksum_field == calculated_checksum) {
        Serial.println("OK");
    } else {
        Serial.println("FAIL");
    }
    Serial.print("  Battery: ");
    Serial.print(battery_percent);
    Serial.println(" %");
    Serial.print("  Battery Voltage: ");
    Serial.print(static_cast<float>(battery_mv) / 1000.0F);
    Serial.println(" V");
    Serial.print("  Signal: ");
    Serial.println(static_cast<int>(signal_strength));
    Serial.print("  Diagnostics: ");
    Serial.println(node_status);
    Serial.print("  Timestamp: ");
    Serial.println(timestamp_ms);
    Serial.print("  Error Code: ");
    Serial.println(error_code);
    Serial.print("  Pairing Flags: ");
    Serial.println(pairing_flags);
    Serial.print("  Security Level: ");
    Serial.println(security_level);
}

}  // namespace Cyber32

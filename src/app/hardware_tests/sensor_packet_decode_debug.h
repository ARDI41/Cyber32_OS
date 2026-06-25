#pragma once

#include <stdint.h>

namespace Cyber32 {

void printSensorPacketEnvelopeDebug(
    const uint8_t* source_mac,
    const uint8_t* data,
    int data_len);

}  // namespace Cyber32

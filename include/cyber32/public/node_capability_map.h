#pragma once

#include <stdint.h>

#include "public_owner_types.h"

namespace Cyber32 {

// Provisional public node-capability link bound for the empty skeleton.
// Final capacity can be revisited when owner-backed mapping links are introduced.
static const uint8_t NODE_CAPABILITY_MAP_MAX_PUBLIC_LINKS = 16;

class NodeCapabilityMap {
public:
    NodeCapabilityMap();

    void reset();
    uint8_t count() const;
    uint8_t capacity() const;
    bool isEmpty() const;
    bool readByIndex(uint8_t index, PublicNodeCapabilityLink& out_link) const;

private:
    PublicNodeCapabilityLink links_[NODE_CAPABILITY_MAP_MAX_PUBLIC_LINKS];
    uint8_t count_;
};

}  // namespace Cyber32

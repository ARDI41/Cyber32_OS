#include "../../include/cyber32/public/node_capability_map.h"

namespace Cyber32 {

NodeCapabilityMap::NodeCapabilityMap() {
    reset();
}

void NodeCapabilityMap::reset() {
    count_ = 0;
    for (uint8_t index = 0; index < NODE_CAPABILITY_MAP_MAX_PUBLIC_LINKS; ++index) {
        links_[index].reset();
    }
}

uint8_t NodeCapabilityMap::count() const {
    return count_;
}

uint8_t NodeCapabilityMap::capacity() const {
    return NODE_CAPABILITY_MAP_MAX_PUBLIC_LINKS;
}

bool NodeCapabilityMap::isEmpty() const {
    return count_ == 0;
}

bool NodeCapabilityMap::readByIndex(uint8_t index, PublicNodeCapabilityLink& out_link) const {
    out_link.reset();
    if (index >= count_) {
        return false;
    }

    out_link = links_[index];
    return true;
}

}  // namespace Cyber32

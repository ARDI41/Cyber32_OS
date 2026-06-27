#include "../../include/cyber32/public/node_capability_map.h"

namespace Cyber32 {

namespace {

bool isValidNodeCapabilityLink(const PublicNodeCapabilityLink& link) {
    return link.link_id != 0 &&
           link.node_id != 0 &&
           link.capability_instance_id != 0 &&
           link.link_visibility_state != PublicVisibilityState::NONE &&
           link.active;
}

}  // namespace

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

bool NodeCapabilityMap::addLink(const PublicNodeCapabilityLink& link) {
    if (!isValidNodeCapabilityLink(link)) {
        return false;
    }
    if (count_ >= NODE_CAPABILITY_MAP_MAX_PUBLIC_LINKS) {
        return false;
    }

    for (uint8_t index = 0; index < count_; ++index) {
        if (links_[index].link_id == link.link_id) {
            return false;
        }
        if (links_[index].node_id == link.node_id &&
            links_[index].capability_instance_id == link.capability_instance_id) {
            return false;
        }
    }

    links_[count_] = link;
    ++count_;
    return true;
}

}  // namespace Cyber32

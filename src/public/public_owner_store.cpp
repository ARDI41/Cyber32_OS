#include "../../include/cyber32/public/public_owner_store.h"

namespace Cyber32 {

PublicOwnerStore::PublicOwnerStore()
    : node_directory_(),
      capability_directory_(),
      node_capability_map_() {
    reset();
}

void PublicOwnerStore::reset() {
    node_directory_.reset();
    capability_directory_.reset();
    node_capability_map_.reset();
}

const NodeDirectory& PublicOwnerStore::nodes() const {
    return node_directory_;
}

const CapabilityDirectory& PublicOwnerStore::capabilities() const {
    return capability_directory_;
}

const NodeCapabilityMap& PublicOwnerStore::nodeCapabilities() const {
    return node_capability_map_;
}

NodeDirectory& PublicOwnerStore::mutableNodes() {
    return node_directory_;
}

CapabilityDirectory& PublicOwnerStore::mutableCapabilities() {
    return capability_directory_;
}

NodeCapabilityMap& PublicOwnerStore::mutableNodeCapabilities() {
    return node_capability_map_;
}

}  // namespace Cyber32

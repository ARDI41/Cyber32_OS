#include "../../include/cyber32/public/public_owner_store.h"

namespace Cyber32 {

PublicOwnerStore::PublicOwnerStore()
    : node_directory_(),
      capability_directory_() {
    reset();
}

void PublicOwnerStore::reset() {
    node_directory_.reset();
    capability_directory_.reset();
}

const NodeDirectory& PublicOwnerStore::nodes() const {
    return node_directory_;
}

const CapabilityDirectory& PublicOwnerStore::capabilities() const {
    return capability_directory_;
}

NodeDirectory& PublicOwnerStore::mutableNodes() {
    return node_directory_;
}

CapabilityDirectory& PublicOwnerStore::mutableCapabilities() {
    return capability_directory_;
}

}  // namespace Cyber32

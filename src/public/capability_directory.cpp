#include "../../include/cyber32/public/capability_directory.h"

namespace Cyber32 {

CapabilityDirectory::CapabilityDirectory() {
    reset();
}

void CapabilityDirectory::reset() {
    count_ = 0;
    for (PublicCapabilityIndex index = 0; index < CAPABILITY_DIRECTORY_MAX_PUBLIC_CAPABILITIES; ++index) {
        records_[index].reset();
    }
}

PublicCapabilityIndex CapabilityDirectory::count() const {
    return count_;
}

PublicCapabilityIndex CapabilityDirectory::capacity() const {
    return CAPABILITY_DIRECTORY_MAX_PUBLIC_CAPABILITIES;
}

bool CapabilityDirectory::isEmpty() const {
    return count_ == 0;
}

bool CapabilityDirectory::readByIndex(PublicCapabilityIndex index, PublicCapabilityRecord& out_record) const {
    out_record.reset();
    if (index >= count_) {
        return false;
    }

    out_record = records_[index];
    return true;
}

}  // namespace Cyber32

#include "../../include/cyber32/api/api_snapshot_builder.h"

namespace Cyber32 {

ApiSnapshotBuilder::ApiSnapshotBuilder() {
}

bool ApiSnapshotBuilder::buildEmpty(ApiSnapshot& out_snapshot) const {
    out_snapshot.reset();
    return true;
}

}  // namespace Cyber32

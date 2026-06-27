#pragma once

#include "api_snapshot_types.h"

namespace Cyber32 {

class ApiSnapshotBuilder {
public:
    ApiSnapshotBuilder();

    bool buildEmpty(ApiSnapshot& out_snapshot) const;
};

}  // namespace Cyber32

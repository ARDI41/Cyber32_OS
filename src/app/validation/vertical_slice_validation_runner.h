#pragma once

#include <stdint.h>

#include "vertical_slice_validation.h"

namespace Cyber32 {

class VerticalSliceValidationRunner {
public:
    VerticalSliceValidationRunner();

    bool begin();
    bool update(uint32_t now_ms);
    bool passed() const;
    const char* lastError() const;

private:
    VerticalSliceValidation validation_;
    bool has_run_;
    bool result_;
};

}  // namespace Cyber32

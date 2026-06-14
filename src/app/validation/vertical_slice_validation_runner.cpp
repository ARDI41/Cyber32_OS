#include "vertical_slice_validation_runner.h"

namespace Cyber32 {

VerticalSliceValidationRunner::VerticalSliceValidationRunner()
    : has_run_(false),
      result_(false) {
}

bool VerticalSliceValidationRunner::begin() {
    has_run_ = false;
    result_ = validation_.begin();
    return result_;
}

bool VerticalSliceValidationRunner::update(uint32_t now_ms) {
    if (has_run_) {
        return result_;
    }

    result_ = validation_.runOnceWithRuntime(now_ms);
    has_run_ = true;
    return result_;
}

bool VerticalSliceValidationRunner::passed() const {
    return validation_.passed();
}

const char* VerticalSliceValidationRunner::lastError() const {
    return validation_.lastError();
}

}  // namespace Cyber32

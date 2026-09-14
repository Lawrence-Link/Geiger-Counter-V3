#include "core/ValueEdit.h"

bool ValueEditorBinding::readReference(const void* context, int32_t& value) {
    if (context == nullptr) return false;
    value = *static_cast<const int32_t*>(context);
    return true;
}

bool ValueEditorBinding::writeReference(void* context, int32_t value) {
    if (context == nullptr) return false;
    *static_cast<int32_t*>(context) = value;
    return true;
}

bool ValueEditorBinding::read(int32_t& value) const {
    return read_ != nullptr && read_(valueContext_, value);
}

bool ValueEditorBinding::write(int32_t value) const {
    return write_ != nullptr && write_(valueContext_, value);
}

void ValueEditorBinding::notify(int32_t value) const {
    if (changed_ != nullptr) changed_(changedContext_, value);
}

ValueEditSession::ValueEditSession(
    int32_t initialValue, ValueEditPolicy policy)
    : originalValue_(initialValue), draftValue_(initialValue), policy_(policy) {}

ValueEditSession::ValueEditSession(
    ValueEditorBinding binding, ValueEditPolicy policy)
    : binding_(binding), policy_(policy) {
    valid_ = binding_.read(originalValue_);
    draftValue_ = originalValue_;
}

bool ValueEditSession::setDraftValue(int32_t value) {
    if (!valid_) return false;
    if (value == draftValue_) return true;
    if (policy_ == ValueEditPolicy::Live && binding_.valid()) {
        if (!binding_.write(value)) return false;
        binding_.notify(value);
    }
    draftValue_ = value;
    return true;
}

bool ValueEditSession::commit() {
    if (!valid_) return false;
    if (policy_ == ValueEditPolicy::CommitOnConfirm && binding_.valid() &&
        draftValue_ != originalValue_) {
        if (!binding_.write(draftValue_)) return false;
        binding_.notify(draftValue_);
    }
    originalValue_ = draftValue_;
    return true;
}

bool ValueEditSession::cancel() {
    if (!valid_) return false;
    if (draftValue_ == originalValue_) return true;
    if (policy_ == ValueEditPolicy::Live && binding_.valid()) {
        if (!binding_.write(originalValue_)) return false;
        binding_.notify(originalValue_);
    }
    draftValue_ = originalValue_;
    return true;
}

#pragma once

#include <stdint.h>

enum class ValueEditPolicy : uint8_t {
    Live,
    CommitOnConfirm,
};

class ValueEditorBinding {
public:
    using ReadFunction = bool (*)(const void* context, int32_t& value);
    using WriteFunction = bool (*)(void* context, int32_t value);
    using ChangedFunction = void (*)(void* context, int32_t value);

    /** @brief Creates an invalid binding with no callbacks. */
    constexpr ValueEditorBinding() = default;

    // valueContext and changedContext are non-owning. They must outlive every
    // read, write and notify performed through this binding.
    /**
     * @brief Creates a binding backed by caller-supplied callbacks.
     * @param valueContext Non-owning context passed to read and write.
     * @param read Callback that reads the current value.
     * @param write Callback that writes a value.
     * @param changed Optional notification callback.
     * @param changedContext Non-owning context passed to changed.
     */
    static constexpr ValueEditorBinding custom(
        void* valueContext,
        ReadFunction read,
        WriteFunction write,
        ChangedFunction changed = nullptr,
        void* changedContext = nullptr) {
        return ValueEditorBinding(
            valueContext, read, write, changed, changedContext);
    }

    /**
     * @brief Creates a binding for a caller-owned int32_t reference.
     * @param value Referenced value, which must outlive the binding.
     * @param changed Optional notification callback.
     * @param changedContext Non-owning callback context.
     */
    static constexpr ValueEditorBinding reference(
        int32_t& value,
        ChangedFunction changed = nullptr,
        void* changedContext = nullptr) {
        return ValueEditorBinding(
            &value, &readReference, &writeReference, changed, changedContext);
    }

    /** @brief Reads the bound value into value. @return false when no reader is valid. */
    bool read(int32_t& value) const;
    /** @param value Value to write. @return false when no writer is valid or it rejects the value. */
    bool write(int32_t value) const;
    /** @brief Notifies the optional changed callback of a value update. */
    void notify(int32_t value) const;
    /** @return Whether both read and write callbacks are present. */
    constexpr bool valid() const {
        return read_ != nullptr && write_ != nullptr;
    }

private:
    /** @brief Stores the non-owning callback contexts and functions. */
    constexpr ValueEditorBinding(
        void* valueContext,
        ReadFunction read,
        WriteFunction write,
        ChangedFunction changed,
        void* changedContext)
        : valueContext_(valueContext),
          changedContext_(changedContext),
          read_(read),
          write_(write),
          changed_(changed) {}

    /** @brief Reads an int32_t referenced through the callback context. */
    static bool readReference(const void* context, int32_t& value);
    /** @brief Writes an int32_t referenced through the callback context. */
    static bool writeReference(void* context, int32_t value);

    void* valueContext_ = nullptr;
    void* changedContext_ = nullptr;
    ReadFunction read_ = nullptr;
    WriteFunction write_ = nullptr;
    ChangedFunction changed_ = nullptr;
};

class ValueEditSession {
public:
    /**
     * @brief Creates a session for a value owned by the caller of the session.
     * @param initialValue Initial and original value.
     * @param policy Write policy used by the session.
     */
    explicit ValueEditSession(
        int32_t initialValue = 0,
        ValueEditPolicy policy = ValueEditPolicy::CommitOnConfirm);
    // A copied binding remains non-owning; its contexts must outlive the session.
    /**
     * @brief Creates a session backed by an external value binding.
     * @param binding Non-owning binding used for reads, writes, and notifications.
     * @param policy Write policy used by the session.
     */
    ValueEditSession(
        ValueEditorBinding binding,
        ValueEditPolicy policy = ValueEditPolicy::CommitOnConfirm);

    /** @param value New draft value. @return false when the session or write is invalid. */
    bool setDraftValue(int32_t value);
    /** @brief Commits the draft according to the selected policy. */
    bool commit();
    /** @brief Restores the original value, writing it back for a live binding. */
    bool cancel();

    /** @return Value captured when the session began or was last committed. */
    int32_t originalValue() const { return originalValue_; }
    /** @return Current uncommitted draft value. */
    int32_t draftValue() const { return draftValue_; }
    /** @return Write policy used by this session. */
    ValueEditPolicy policy() const { return policy_; }
    /** @return Whether the session's external binding was read successfully. */
    bool valid() const { return valid_; }
    /** @return Whether an external binding is configured. */
    bool hasExternalBinding() const { return binding_.valid(); }

private:
    ValueEditorBinding binding_{};
    int32_t originalValue_ = 0;
    int32_t draftValue_ = 0;
    ValueEditPolicy policy_ = ValueEditPolicy::CommitOnConfirm;
    bool valid_ = true;
};

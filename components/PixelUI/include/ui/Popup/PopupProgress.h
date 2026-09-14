/*
 * Copyright (C) 2025 Lawrence Link
 */

#pragma once

#include "PopupBase.h"
#include <stddef.h>
#include "config.h"
#include "core/Callbacks.h"
#include "core/NumericFormatter.h"
#include "core/NumericRange.h"
#include "core/ValueEdit.h"

enum class PopupProgressMode : uint8_t {
    ReadOnly,
    Editable,
};

/**
 * @class PopupProgress
 * @brief A popup displaying a progress bar.
 */
class PopupProgress : public PopupBase {
private:
    NumericRange range_;
    PercentageFormat defaultPercentage_;
    NumericFormatter formatter_;
    // title_, formatter context, binding contexts in ownedSession_, and an
    // injected session are non-owning and must outlive this Popup.
    const char* title_;
    ValueCallback compatibilityCallback_;
    ValueEditSession ownedSession_;
    ValueEditSession* session_ = nullptr;
    ValueEditorBinding sourceBinding_{};
    PopupProgressMode mode_ = PopupProgressMode::ReadOnly;

    /** @brief Updates the active editing session and refreshes the display value. */
    bool updateDraft(int32_t value);
    /** @brief Commits the active editing session. */
    bool commitEditing();
    /** @brief Cancels the active editing session and restores its original value. */
    bool cancelEditing();

public:
    /**
     * @brief Creates a progress popup using an externally owned edit session.
     * @param session Non-owning session that must outlive this popup.
     */
    PopupProgress(PixelUI& ui, uint16_t width, uint16_t height,
                  const NumericRange& range, NumericFormatter formatter,
                  ValueEditSession& session, const char* title = "",
                  uint16_t duration = 3000,
                  PopupProgressMode mode = PopupProgressMode::ReadOnly);
    /**
     * @brief Creates a progress popup using an external value binding.
     * @param binding Non-owning value binding.
     * @param callback Optional callback invoked after value changes.
     */
    PopupProgress(PixelUI& ui, uint16_t width, uint16_t height,
                  const NumericRange& range, NumericFormatter formatter,
                  ValueEditorBinding binding, const char* title,
                  uint16_t duration, ValueCallback callback = nullptr,
                  ValueEditPolicy policy = ValueEditPolicy::CommitOnConfirm,
                  PopupProgressMode mode = PopupProgressMode::ReadOnly);
    /** @brief Creates a progress popup with an internal value-only session. */
    PopupProgress(PixelUI& ui, uint16_t width, uint16_t height,
                  const NumericRange& range, NumericFormatter formatter,
                  int32_t initialValue, const char* title = "",
                  uint16_t duration = 3000,
                  PopupProgressMode mode = PopupProgressMode::ReadOnly);
    /** @brief Destroys the progress popup and releases no external values. */
    ~PopupProgress() = default;

    /** @brief Draws the formatted value and progress indicator. */
    void drawContent(const PopupContentBounds& bounds) override;
    /** @return Whether the event changed, committed, or cancelled editing. */
    bool handleContentInput(InputEvent event) override;
};

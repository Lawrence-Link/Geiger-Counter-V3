/*
 * Copyright (C) 2025 Lawrence Link
 */

#pragma once

#include "core/Callbacks.h"
#include "core/NumericFormatter.h"
#include "core/NumericRange.h"
#include "core/ValueEdit.h"
#include "config.h"
#include <etl/queue.h>
#include <etl/utility.h>
#include <etl/variant.h>
#include <etl/variant_pool.h>

#if PIXELUI_USE_POPUP_INFO
#include "PopupInfo.h"
#endif

#if PIXELUI_USE_POPUP_PROGRESS
#include "PopupProgress.h"
#endif

#if PIXELUI_USE_POPUP_VALUE_DIGITS
#include "PopupValueDigits.h"
#endif

#if PIXELUI_USE_POPUP_KEYBOARD
#include "PopupKeyboard.h"
#endif

class PixelUI;

#if PIXELUI_USE_POPUP

struct RequestEnvelope {
    uint16_t width = 0;
    uint16_t height = 0;
    uint16_t duration = 0;
};

#if PIXELUI_USE_POPUP_INFO
struct InfoRequest {
    RequestEnvelope envelope{};
    // All pointers are non-owning and must survive pending plus active use.
    const char* text = nullptr;
    const char* title = nullptr;
    const uint8_t* font = nullptr;
};
#endif

#if PIXELUI_USE_POPUP_PROGRESS
struct ProgressRequest {
    RequestEnvelope envelope{};
    ValueEditPolicy policy = ValueEditPolicy::CommitOnConfirm;
    PopupProgressMode mode = PopupProgressMode::ReadOnly;
    NumericRange range{};
    // Formatter and binding contexts are non-owning.
    NumericFormatter formatter{};
    ValueEditorBinding binding{};
    const char* title = nullptr;
    // The callback object is owned, but any references captured by it are not.
    ValueCallback callback{};
};
#endif

#if PIXELUI_USE_POPUP_VALUE_DIGITS
struct ValueDigitsRequest {
    RequestEnvelope envelope{};
    uint8_t digitCount = 0U;
    ValueEditPolicy policy = ValueEditPolicy::CommitOnConfirm;
    // Binding contexts and title are non-owning.
    ValueEditorBinding binding{};
    const char* title = nullptr;
    // The callback object is owned, but any references captured by it are not.
    ValueCallback callback{};
};
#endif

#if PIXELUI_USE_POPUP_KEYBOARD
struct KeyboardRequest {
    RequestEnvelope envelope{};
    // output is non-owning and must survive pending plus active use.
    char* output = nullptr;
    size_t outputCapacity = 0U;
    VoidCallback commitCallback{};
};
#endif

using PopupRequest = etl::variant<
#if PIXELUI_USE_POPUP_INFO
    InfoRequest
    #if PIXELUI_USE_POPUP_PROGRESS || PIXELUI_USE_POPUP_VALUE_DIGITS || PIXELUI_USE_POPUP_KEYBOARD
    ,
    #endif
#endif
#if PIXELUI_USE_POPUP_PROGRESS
    ProgressRequest
    #if PIXELUI_USE_POPUP_VALUE_DIGITS || PIXELUI_USE_POPUP_KEYBOARD
    ,
    #endif
#endif
#if PIXELUI_USE_POPUP_VALUE_DIGITS
    ValueDigitsRequest
    #if PIXELUI_USE_POPUP_KEYBOARD
    ,
    #endif
#endif
#if PIXELUI_USE_POPUP_KEYBOARD
    KeyboardRequest
#endif
>;

/**
 * @brief Owns one active Popup and a fixed-capacity FIFO of pending requests.
 *
 * Non-owning request targets must remain valid until the request is removed or
 * its active Popup is destroyed. clearPopups() ends that required lifetime.
 */
class PopupManager {
private:
    static_assert(MAX_POPUP_NUM > 0, "Popup capacity must include one active slot");
    static constexpr size_t MAX_PENDING_POPUP_NUM = MAX_POPUP_NUM - 1U;

    using ActivePool = etl::variant_pool<
        1
#if PIXELUI_USE_POPUP_INFO
        , PopupInfo
#endif
#if PIXELUI_USE_POPUP_PROGRESS
        , PopupProgress
#endif
#if PIXELUI_USE_POPUP_VALUE_DIGITS
        , PopupValueDigits
#endif
#if PIXELUI_USE_POPUP_KEYBOARD
        , PopupKeyboard
#endif
    >;

    PixelUI& ui_;
    ActivePool activePool_;
    etl::queue<PopupRequest, MAX_PENDING_POPUP_NUM> requests_;
    IPopup* active_ = nullptr;
    bool dispatching_ = false;

    /** @return Whether width and height are both non-zero. */
    static bool validEnvelope(const RequestEnvelope& envelope);
    /** @brief Queues a request when capacity and dispatch state permit it. */
    bool enqueue(PopupRequest&& request);
    /** @brief Activates the oldest pending request when no popup is active. */
    void activateNext();
    /** @brief Destroys the active popup in the fixed variant pool. */
    void destroyActive();
#if PIXELUI_USE_POPUP_INFO
    /** @brief Constructs an active information popup from a pending request. */
    void activate(InfoRequest& request);
#endif
#if PIXELUI_USE_POPUP_PROGRESS
    /** @brief Constructs an active progress popup from a pending request. */
    void activate(ProgressRequest& request);
#endif
#if PIXELUI_USE_POPUP_VALUE_DIGITS
    /** @brief Constructs an active digit-edit popup from a pending request. */
    void activate(ValueDigitsRequest& request);
#endif
#if PIXELUI_USE_POPUP_KEYBOARD
    /** @brief Constructs an active compact keyboard from a pending request. */
    void activate(KeyboardRequest& request);
#endif

public:
    /** @brief Creates an empty popup manager bound to the supplied UI. */
    explicit PopupManager(PixelUI& ui) : ui_(ui) {}
    /** @brief Clears pending requests and destroys the active popup. */
    ~PopupManager();

    /** @brief Copy construction is disabled because the manager owns popup storage. */
    PopupManager(const PopupManager&) = delete;
    /** @brief Copy assignment is disabled because the manager owns popup storage. */
    PopupManager& operator=(const PopupManager&) = delete;

#if PIXELUI_USE_POPUP_INFO
    /** @brief Queues an informational popup request. @return false for invalid input or full capacity. */
    bool enqueueInfo(uint16_t width, uint16_t height,
                     const char* text, const char* title, uint16_t duration,
                     const uint8_t* font = PIXELUI_FONT_TEXT);
#endif
#if PIXELUI_USE_POPUP_PROGRESS
    /** @brief Queues a progress popup using explicit range, formatter, and binding. */
    bool enqueueProgress(uint16_t width, uint16_t height,
                         ValueEditorBinding binding,
                         const NumericRange& range,
                         NumericFormatter formatter,
                         const char* title, uint16_t duration,
                         ValueCallback callback = nullptr,
                         ValueEditPolicy policy = ValueEditPolicy::CommitOnConfirm,
                         PopupProgressMode mode = PopupProgressMode::ReadOnly);
    /** @brief Queues a progress popup backed by an int32_t reference. */
    bool enqueueProgress(uint16_t width, uint16_t height,
                         int32_t& value, int32_t minValue, int32_t maxValue,
                         const char* title, uint16_t duration,
                         ValueCallback callback = nullptr,
                         PopupProgressMode mode = PopupProgressMode::ReadOnly);
#endif
#if PIXELUI_USE_POPUP_VALUE_DIGITS
    /** @brief Queues a digit editor using an explicit value binding. */
    bool enqueueValueDigits(uint16_t width, uint16_t height,
                            ValueEditorBinding binding, uint8_t digitCount,
                            const char* title, uint16_t duration,
                            ValueCallback callback = nullptr,
                            ValueEditPolicy policy = ValueEditPolicy::CommitOnConfirm);
    /** @brief Queues a digit editor backed by an int32_t reference. */
    bool enqueueValueDigits(uint16_t width, uint16_t height,
                            int32_t& value, uint8_t digitCount,
                            const char* title, uint16_t duration,
                            ValueCallback callback = nullptr);
#endif
#if PIXELUI_USE_POPUP_KEYBOARD
    /** @brief Queues a fixed-capacity multi-tap keyboard. */
    bool enqueueKeyboard(uint16_t width, uint16_t height,
                         char* output, size_t outputCapacity,
                         uint16_t duration = 0U,
                         VoidCallback commitCallback = nullptr);
#endif

    /** @brief Removes pending requests and destroys the active popup. */
    void clearPopups();
    /** @brief Draws the active popup when one exists. */
    void drawPopups();
    /** @brief Advances the active popup and activates queued requests as needed. */
    void updatePopups(uint32_t currentTime);
    /** @return Delay until the active popup or next queued request needs service. */
    uint32_t nextWakeupMs(uint32_t currentTime, uint32_t frameIntervalMs) const;
    /** @return Whether the active popup consumed the input event. */
    bool handleTopPopupInput(InputEvent event);

    /** @return Number of active and pending popup requests. */
    size_t getPopupCounts() const {
        return requests_.size() + ((active_ != nullptr) ? 1U : 0U);
    }
    /** @return Number of pending popup requests. */
    size_t pendingCount() const { return requests_.size(); }
    /** @return Whether a popup is currently active. */
    bool hasActivePopup() const { return active_ != nullptr; }
};

#endif

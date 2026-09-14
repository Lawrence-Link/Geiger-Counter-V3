/*
 * Copyright (C) 2025 Lawrence Link
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "ui/ListView/ListView.h"
#include "core/animation/animation.h"

ListView::ListView(PixelUI& ui, ListItem* itemList, int length)
    : IApplication(true)
    , m_ui(ui)
    , m_itemList(itemList)
    , m_itemLength(length - 1)
#if PIXELUI_USE_LABEL_SCROLL
    , selectedItemTitle_(ui, TITLE_X, 0, "", POS::BOTTOM,
                         PIXELUI_FONT_TEXT)
#endif
{
#if PIXELUI_USE_LABEL_SCROLL
    selectedItemTitle_.setOverflow(Label::Overflow::AutoScroll);
#endif
}

ListView::~ListView() {
    cancelAllOwnedAnimations();
}

/** @brief ListView::cancelLoadAnimations. */
void ListView::cancelLoadAnimations() {
    for (AnimationHandle handle : loadAnimations_) m_ui.cancelAnimation(handle);
    loadAnimations_.clear();
}

/** @brief ListView::cancelToggleAnimation. */
void ListView::cancelToggleAnimation() {
    m_ui.cancelAnimation(switchAnimState_.handle);
    switchAnimState_ = {};
}

/** @brief ListView::toggleBoxXFor. */
int32_t ListView::toggleBoxXFor(const ListItem& item) const {
        const bool* toggle = item.accessory.toggleValue();
    if (toggle == nullptr) return 0;
    return switchAnimState_.item == &item
        ? switchAnimState_.boxX
        : (*toggle ? 7 : 0);
}

/** @brief ListView::clearNonInitialAnimations. */
void ListView::clearNonInitialAnimations() {
    for (AnimationHandle handle : viewAnimations_) m_ui.cancelAnimation(handle);
    viewAnimations_.clear();
}

/** @brief ListView::cancelAllOwnedAnimations. */
void ListView::cancelAllOwnedAnimations() {
    clearNonInitialAnimations();
    cancelLoadAnimations();
    cancelToggleAnimation();
}

bool ListView::animateOwned(
    int32_t& value, int32_t target, uint32_t duration,
    EasingType easing, PROTECTION protection) {
    if (viewAnimations_.full()) {
        value = target;
        return false;
    }
    AnimationHandle handle = INVALID_ANIMATION_HANDLE;
    if (!m_ui.animate(
            value, target, duration, easing, protection, &handle)) {
        value = target;
        return false;
    }
    viewAnimations_.push_back(handle);
    return true;
}

bool ListView::animateOwnedCallback(
    int32_t start, int32_t target, uint32_t duration,
    EasingType easing, ValueCallback callback, PROTECTION protection) {
    if (viewAnimations_.full()) {
        if (callback) callback(target);
        return false;
    }
    ValueCallback fallback = callback;
    AnimationHandle handle = INVALID_ANIMATION_HANDLE;
    if (!m_ui.animateCallback(
            start, target, duration, easing, etl::move(callback),
            protection, &handle)) {
        if (fallback) fallback(target);
        return false;
    }
    viewAnimations_.push_back(handle);
    return true;
}

/**
 * @brief Called when the ListView is entered.
 * @param exitCallback Callback function to call on exit.
 *
 * Initializes font, scroll position, cursor, and initial load animation.
 * Sets up switch item animation states and triggers scrollbar animation.
 */
void ListView::onEnter(ExitCallback exitCallback){
    IApplication::onEnter(exitCallback);
    cancelAllOwnedAnimations();
#if PIXELUI_USE_LABEL_SCROLL
    selectedItemTitle_.onOffload();
#endif

    Canvas& canvas = m_ui.getCanvas();
    canvas.setFont(PIXELUI_FONT_TEXT);
    FontHeight = canvas.getFontAscent() - canvas.getFontDescent();
    
    topVisibleIndex_ = 0;
    m_ui.getCanvas().camera().setContentHeight(m_ui.getDisplayHeight());
    m_ui.scrollCanvasTo(0);
    currentCursor = 0;
    isInitialLoad_ = true;
    
    onLoad(); // Initialize item data before calculating box states

    // Reset item load animation states
    for (int i = 0; i < VISIBLE_ITEM_COUNT + 1; i++) {
        itemLoadAnimations_[i] = 0;
    }
    
    startLoadAnimation();
    scrollToTarget();
#if PIXELUI_USE_LABEL_SCROLL
    selectedItemTitle_.onLoadImmediately();
#endif
}

/**
 * @brief Start the initial item load animations.
 *
 * Each visible item is animated with a staggered delay. The last animation
 * clears the PROTECTED marks and sets `isInitialLoad_` to false.
 */
void ListView::startLoadAnimation() {
    cancelLoadAnimations();
    isInitialLoad_ = true;
    int maxVisible = etl::min(VISIBLE_ITEM_COUNT + 1, (int32_t)(m_itemLength + 1));

    for (int i = 0; i < maxVisible; i++) {
        int duration = 250 + i * 60;
        bool isLastAnimation = (i == maxVisible - 1);
        auto callback = [this, i, isLastAnimation](int32_t value) {
            this->itemLoadAnimations_[i] = value;
            if (isLastAnimation && value >= FIXED_POINT_ONE) { 
                this->isInitialLoad_ = false;
            }
        };

        AnimationHandle handle = INVALID_ANIMATION_HANDLE;
        const bool started = m_ui.animateCallback(
            0,
            FIXED_POINT_ONE,
            duration,
            EasingType::EASE_IN_OUT_CUBIC,
            callback,
            PROTECTION::NOT_PROTECTED,
            &handle);
        if (started && !loadAnimations_.full()) {
            loadAnimations_.push_back(handle);
        } else if (!started) {
            itemLoadAnimations_[i] = FIXED_POINT_ONE;
            if (isLastAnimation) isInitialLoad_ = false;
        }
    }
}

/**
 * @brief Determines whether scrolling is required for the new cursor.
 * @param newCursor The new cursor index.
 * @return True if scrolling is needed, false otherwise.
 */
bool ListView::shouldScroll(int newCursor) {
    return newCursor < topVisibleIndex_ ||
           newCursor >= topVisibleIndex_ + VISIBLE_ITEM_COUNT;
}

/**
 * @brief Update the scroll position based on current cursor.
 *
 * Calculates new top visible index and triggers smooth scroll animation
 * using `m_ui.animate`.
 */
void ListView::updateScrollPosition() {
    if (!shouldScroll(currentCursor)) return;

    const int32_t rowHeight = FontHeight + SPACING;
    int32_t newTopIndex = topVisibleIndex_;
    if (currentCursor < topVisibleIndex_) {
        newTopIndex = currentCursor;
    } else if (currentCursor >= topVisibleIndex_ + VISIBLE_ITEM_COUNT) {
        newTopIndex = currentCursor - VISIBLE_ITEM_COUNT + 1;
    }

    const int32_t maxTopIndex = etl::max(
        (int32_t)0, m_itemLength + 1 - VISIBLE_ITEM_COUNT);
    newTopIndex = etl::max((int32_t)0,
                           etl::min(newTopIndex, maxTopIndex));
    if (newTopIndex == topVisibleIndex_) return;

    topVisibleIndex_ = newTopIndex;
    const int32_t targetY = topVisibleIndex_ * rowHeight;
    const int32_t startY = m_ui.getCanvas().camera().storedY();
    animateOwnedCallback(
        startY, targetY, 150, EasingType::EASE_OUT_CUBIC,
        [this](int32_t value) { m_ui.getCanvas().camera().setY(value); },
        PROTECTION::PROTECTED);
}

/**
 * @brief Calculate the Y coordinate of an item on screen.
 * @param itemIndex Index of the item.
 * @return Screen Y position of the item including scroll offset.
 */
int32_t ListView::calculateItemY(int itemIndex) {
    return TOP_MARGIN + itemIndex * (FontHeight + SPACING) +
           m_ui.getCanvas().getFontAscent();
}

int32_t ListView::calculateTitleRight(
    const ListItem& item, char* valueBuffer, size_t valueBufferSize) const {
    Canvas& canvas = m_ui.getCanvas();
    const int32_t displayWidth = m_ui.getDisplayWidth();
    int32_t right = displayWidth - 15;

    if (valueBuffer != nullptr && valueBufferSize != 0U) {
        valueBuffer[0] = '\0';
    }

    switch (item.accessory.kind()) {
        case ListItemAccessory::Kind::Toggle:
            if (item.accessory.toggleValue() != nullptr) {
                right = displayWidth - 42 - TITLE_ACCESSORY_GAP;
            }
            break;

        case ListItemAccessory::Kind::Text: {
            const char* text = item.accessory.textValue();
            if (text != nullptr) {
                right = displayWidth - canvas.getUTF8Width(text) - 4 -
                        TITLE_ACCESSORY_GAP;
            }
            break;
        }

        case ListItemAccessory::Kind::Value:
            if (item.accessory.formatValue(valueBuffer, valueBufferSize)) {
                right = displayWidth - canvas.getUTF8Width(valueBuffer) - 8 -
                        TITLE_ACCESSORY_GAP;
            }
            break;

        case ListItemAccessory::Kind::None:
            break;
    }

    return etl::max(TITLE_X + 1, right);
}

/** @brief ListView::updateSelectedItemTitle. */
int32_t ListView::updateSelectedItemTitle() {
    if (m_itemLength < 0 || currentCursor < 0 ||
        currentCursor > m_itemLength) {
#if PIXELUI_USE_LABEL_SCROLL
        selectedItemTitle_.setText("");
#endif
        return TITLE_X + 1;
    }

    char valueBuffer[MAX_TEXT_LENGTH + 1]{};
    const int32_t titleRight = calculateTitleRight(
        m_itemList[currentCursor], valueBuffer, sizeof(valueBuffer));
#if PIXELUI_USE_LABEL_SCROLL
    selectedItemTitle_.setViewportWidth(
        static_cast<uint16_t>(titleRight - TITLE_X));
    selectedItemTitle_.setText(m_itemList[currentCursor].title);
#endif
    return titleRight;
}

/**
 * @brief Scroll UI to ensure target item is visible and animate cursor/progress bar.
 * @param target Target item index.
 */
void ListView::scrollToTarget(){
    if (m_itemLength < 0 || currentCursor < 0 || currentCursor > m_itemLength) {
        m_ui.scrollCanvasTo(0);
        updateSelectedItemTitle();
        return;
    }

    const int32_t contentHeight = TOP_MARGIN +
        (m_itemLength + 1) * (FontHeight + SPACING);
    m_ui.getCanvas().camera().setContentHeight(contentHeight);
    updateScrollPosition();
    
    Canvas& canvas = m_ui.getCanvas();
    const int32_t titleRight = updateSelectedItemTitle();
    int32_t targetCursorY = TOP_MARGIN + currentCursor * (FontHeight + SPACING) - 1;
    
    const int32_t displayHeight = m_ui.getDisplayHeight();
    animateOwned(CursorY, targetCursorY, 150, EasingType::EASE_IN_OUT_CUBIC);
    animateOwned(CursorWidth,
                 etl::min(
                     canvas.getUTF8Width(m_itemList[currentCursor].title) + 6,
                     titleRight + 2 - CURSOR_X),
                 500, EasingType::EASE_OUT_CUBIC);
    animateOwned(progress_bar_top,
                 static_cast<int32_t>(
                     (static_cast<int64_t>(currentCursor) * displayHeight) /
                     (m_itemLength + 1)) + 1,
                 400, EasingType::EASE_OUT_CUBIC, PROTECTION::PROTECTED);
    animateOwned(progress_bar_bottom,
                 displayHeight / (m_itemLength + 1),
                 400, EasingType::EASE_OUT_CUBIC, PROTECTION::PROTECTED);
}

/**
 * @brief Move cursor up by one item and scroll if needed.
 */
void ListView::navigateUp() {
    if (currentCursor != 0) clearNonInitialAnimations();
    if (currentCursor > 0) {
        currentCursor--;
        scrollToTarget();
    }
}

/**
 * @brief Move cursor down by one item and scroll if needed.
 */
void ListView::navigateDown() {
    if (currentCursor != m_itemLength) clearNonInitialAnimations();
    if (currentCursor < m_itemLength) {
        currentCursor++;
        scrollToTarget();
    }
}

/**
 * @brief Trigger action associated with current item selection.
 *
 * Handles switching toggle items, entering nested lists, and calling
 * item-specific functions.
 */
void ListView::selectCurrent(){
    if (currentCursor == 0) { returnToPreviousContext(); return; }

    if (m_itemList[currentCursor].nextList){  
        cancelAllOwnedAnimations();
        m_history_stack.push_back(etl::make_pair(etl::make_pair(m_itemList, m_itemLength), currentCursor));
        // Guard against underflow if nextListLength is zero
        int32_t nextLen = m_itemList[currentCursor].nextListLength;
        if (nextLen <= 0) nextLen = 0;
        m_itemLength = nextLen - 1;
        m_itemList = m_itemList[currentCursor].nextList;
        currentCursor = 0;
        m_ui.scrollCanvasTo(0);
        m_ui.markFading();
        startLoadAnimation();
        scrollToTarget();
        return;
    }

    if (bool* switchValPtr =
            m_itemList[currentCursor].accessory.toggleValue()) {
        bool currentState = *switchValPtr;
        int32_t endX = currentState ? 0 : 7;

        ListItem* const targetItem = &m_itemList[currentCursor];
        cancelToggleAnimation();
        switchAnimState_.item = targetItem;
        switchAnimState_.boxX = currentState ? 7 : 0;
        *switchValPtr = !currentState;

        auto callback = [this, targetItem, endX](int32_t value) {
            if (switchAnimState_.item == targetItem) {
                switchAnimState_.boxX = value;
                if (value == endX) switchAnimState_ = {};
            }
        };
        if (!m_ui.animateCallback(
            switchAnimState_.boxX,
            endX,
            200,
            EasingType::EASE_IN_OUT_CUBIC,
            callback,
            PROTECTION::PROTECTED,
            &switchAnimState_.handle)) {
            switchAnimState_ = {};
        }
    }

    if (m_itemList[currentCursor].pFunc) { 
        m_itemList[currentCursor].pFunc(); 
        if (m_itemList[currentCursor].use_fade) {
            m_ui.markFading();
        }
    }
}

/**
 * @brief Return to the previous ListView context or exit if none exists.
 */
void ListView::returnToPreviousContext() {
    if (!m_history_stack.empty()){
        cancelAllOwnedAnimations();
        auto parent_state = m_history_stack.back();
        m_history_stack.pop_back();
        m_itemList = parent_state.first.first;
        m_itemLength = parent_state.first.second;
        currentCursor = parent_state.second;

        m_ui.markFading();
        m_ui.scrollCanvasTo(0);
        startLoadAnimation();
        scrollToTarget();
        return;
    }
    else { requestExit(); }
}

/**
 * @brief Navigate left: same as returning to previous context.
 */
void ListView::navigateLeft() { returnToPreviousContext(); }

/**
 * @brief Navigate right: same as selecting current item.
 */
void ListView::navigateRight() { selectCurrent(); }

/**
 * @brief Handle user input events
 * @param event Input event enum
 * @return true if handled, false otherwise
 */
bool ListView::handleInput(InputEvent event) {
    switch (event) {
        #ifdef LISTVIEW_NAVI_UP 
        case LISTVIEW_NAVI_UP: navigateUp(); return true; 
        #endif

        #ifdef LISTVIEW_NAVI_DOWN
        case LISTVIEW_NAVI_DOWN: navigateDown(); return true;
        #endif

        #ifdef LISTVIEW_NAVI_RIGHT
        case LISTVIEW_NAVI_RIGHT: navigateRight(); return true;
        #endif

        #ifdef LISTVIEW_NAVI_BACK
        case LISTVIEW_NAVI_BACK: requestExit(); return true;
        #endif

        #ifdef LISTVIEW_NAVI_LEFT
        case LISTVIEW_NAVI_LEFT: navigateLeft(); return true;
        #endif

        #ifdef LISTVIEW_NAVI_SELECT
        case LISTVIEW_NAVI_SELECT: selectCurrent(); return true;
        #endif
        default:
            return event == InputEvent::UP || event == InputEvent::DOWN;
    }
}

/**
 * @brief Draw the cursor rectangle and navigation hints.
 */
void ListView::drawCursor() {
    Canvas& canvas = m_ui.getCanvas();
    canvas.setDrawColor(2);
    canvas.drawRBox(CURSOR_X, CursorY - 2, CursorWidth, FontHeight + 3, 0);
    canvas.setDrawColor(1);

    U8G2& overlay = canvas.rawDisplay();
    const char* hint = currentCursor ? ">" : "<";
    overlay.drawStr(m_ui.getDisplayWidth() - canvas.getUTF8Width(hint) - 5,
                    m_ui.getDisplayHeight(), hint);
}

/**
 * @brief Called when resuming ListView.
 */
void ListView::onResume() {
    cancelLoadAnimations();
    isInitialLoad_ = false;
    updateSelectedItemTitle();
#if PIXELUI_USE_LABEL_SCROLL
    selectedItemTitle_.onLoadImmediately();
#endif
}

/**
 * @brief Called when ListView is paused.
 */
void ListView::onPause() {
#if PIXELUI_USE_LABEL_SCROLL
    selectedItemTitle_.onOffload();
#endif
}

/**
 * @brief Called when ListView is exited.
 *
 * Saves state and clears animations.
 */
void ListView::onExit() {
#if PIXELUI_USE_LABEL_SCROLL
    selectedItemTitle_.onOffload();
#endif
    cancelAllOwnedAnimations();
    onSave();
}

/**
 * @brief Draw all visible items and UI elements.
 */
void ListView::draw() {
    Canvas& canvas = m_ui.getCanvas();
    canvas.setFont(PIXELUI_FONT_TEXT);
    const int32_t rowHeight = FontHeight + SPACING;
    const int32_t cameraY = canvas.camera().storedY();
    int startIndex = etl::max((int32_t)0, topVisibleIndex_ - 2);
    int endIndex = etl::min(m_itemLength, topVisibleIndex_ + VISIBLE_ITEM_COUNT + 2);
    canvas.setContentHeight(TOP_MARGIN + (m_itemLength + 1) * rowHeight);
    
    for (int itemIndex = startIndex; itemIndex <= endIndex; itemIndex++) {
        int32_t itemY = calculateItemY(itemIndex);
        
        const int32_t screenY = itemY - cameraY;
        if (screenY >= -FontHeight && screenY <= m_ui.getDisplayHeight() + FontHeight) {
            int32_t drawX = TITLE_X;
            if (isInitialLoad_) {
                int animIndex = itemIndex - topVisibleIndex_;
                if (animIndex >= 0 && animIndex < VISIBLE_ITEM_COUNT + 1) {
                    int32_t loadProgress = itemLoadAnimations_[animIndex];
                    drawX = TITLE_X +
                        (FIXED_POINT_ONE - loadProgress) * 30 / FIXED_POINT_ONE;
                }
            }

            const ListItemAccessory& accessory =
                m_itemList[itemIndex].accessory;
            char valueBuffer[MAX_TEXT_LENGTH + 1]{};
            const int32_t titleRight = calculateTitleRight(
                m_itemList[itemIndex], valueBuffer, sizeof(valueBuffer));
            if (drawX < titleRight) {
#if PIXELUI_USE_LABEL_SCROLL
                if (itemIndex == currentCursor) {
                    const uint16_t viewportWidth = static_cast<uint16_t>(
                        titleRight - drawX);
                    if (selectedItemTitle_.getViewportWidth() != viewportWidth) {
                        selectedItemTitle_.setViewportWidth(viewportWidth);
                    }
                    const WidgetRenderContext context{
                        drawX - TITLE_X,
                        itemY,
                        {canvas.camera().x(), cameraY,
                         m_ui.getDisplayWidth(), m_ui.getDisplayHeight()}
                    };
                    selectedItemTitle_.draw(context);
                } else {
#endif
                    canvas.setClipWindow(
                        drawX, itemY - FontHeight, titleRight, itemY + 1);
                    canvas.drawUTF8(
                        drawX, itemY, m_itemList[itemIndex].title);
                    canvas.setMaxClipWindow();
#if PIXELUI_USE_LABEL_SCROLL
                }
#endif
            }

            switch (accessory.kind()) {
                case ListItemAccessory::Kind::Toggle: {
                    bool* toggle = accessory.toggleValue();
                    if (toggle == nullptr) break;
                    canvas.drawRFrame(
                        m_ui.getDisplayWidth() - 42, itemY - 9, 14, 8, 1);
                    const int32_t currentSwitchBoxX =
                        toggleBoxXFor(m_itemList[itemIndex]);
                    canvas.drawRBox(
                        m_ui.getDisplayWidth() - 42 + currentSwitchBoxX,
                        itemY - 9,
                        7,
                        8,
                        2);
                    canvas.drawUTF8(
                        m_ui.getDisplayWidth() - 25,
                        itemY - 1,
                        *toggle ? "ON" : "OFF");
                    break;
                }
                case ListItemAccessory::Kind::Text: {
                    const char* text = accessory.textValue();
                    if (text != nullptr) {
                        canvas.drawStr(
                            m_ui.getDisplayWidth() - canvas.getUTF8Width(text) - 4,
                            itemY,
                            text);
                    }
                    break;
                }
                case ListItemAccessory::Kind::Value: {
                    if (valueBuffer[0] != '\0') {
                        canvas.drawStr(
                            m_ui.getDisplayWidth() -
                                canvas.getUTF8Width(valueBuffer) - 8,
                            itemY,
                            valueBuffer);
                    }
                    break;
                }
                case ListItemAccessory::Kind::None:
                    break;
            }
        }
    }

    // Draw progress bar and cursor
    canvas.rawDisplay().drawVLine(
        m_ui.getDisplayWidth() - 2, progress_bar_top, progress_bar_bottom);
    drawCursor();
}

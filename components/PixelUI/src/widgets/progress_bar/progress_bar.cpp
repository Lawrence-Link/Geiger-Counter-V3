#include "widgets/progress_bar/progress_bar.h"

#include "PixelUI.h"

/** @brief ProgressBar::ProgressBar. */
ProgressBar::ProgressBar(PixelUI& ui, int16_t x, int16_t y,
                         uint16_t width, uint16_t height,
                         int32_t percent, bool useAnimation)
    : ui_(ui),
      percent_(clampPercent(percent)),
      displayedPercent_(useAnimation ? 0 : percent_),
      useAnimation_(useAnimation) {
    setWidgetBounds({x, y, width, height});
}

/** @brief ProgressBar::~ProgressBar. */
ProgressBar::~ProgressBar() {
    cancelAnimation();
}

/** @brief ProgressBar::onLoad. */
void ProgressBar::onLoad() {
    cancelAnimation();
    displayedPercent_ = useAnimation_ ? 0 : percent_;
    updateDisplayedPercent(PROTECTION::PROTECTED);
}

/** @brief ProgressBar::onOffload. */
void ProgressBar::onOffload() {
    cancelAnimation();
    displayedPercent_ = percent_;
    ui_.markDirty();
}

/** @brief ProgressBar::setPercent. */
void ProgressBar::setPercent(int32_t percent) {
    const int32_t clamped = clampPercent(percent);
    if (clamped == percent_) return;
    percent_ = clamped;
    updateDisplayedPercent();
}

/** @brief ProgressBar::setPercentImmediate. */
void ProgressBar::setPercentImmediate(int32_t percent) {
    cancelAnimation();
    percent_ = clampPercent(percent);
    displayedPercent_ = percent_;
    ui_.markDirty();
}

/** @brief ProgressBar::setPosition. */
void ProgressBar::setPosition(int16_t x, int16_t y) {
    const FocusBox bounds = getLocalBounds();
    setWidgetBounds({x, y, bounds.w, bounds.h});
    ui_.markDirty();
}

/** @brief ProgressBar::setSize. */
void ProgressBar::setSize(uint16_t width, uint16_t height) {
    const FocusBox bounds = getLocalBounds();
    setWidgetBounds({bounds.x, bounds.y, width, height});
    ui_.markDirty();
}

/** @brief ProgressBar::clampPercent. */
int32_t ProgressBar::clampPercent(int32_t percent) {
    if (percent < 0) return 0;
    if (percent > MAX_PERCENT) return MAX_PERCENT;
    return percent;
}

/** @brief ProgressBar::cancelAnimation. */
void ProgressBar::cancelAnimation() {
    ui_.cancelAnimation(animation_);
    animation_ = INVALID_ANIMATION_HANDLE;
}

/** @brief ProgressBar::updateDisplayedPercent. */
void ProgressBar::updateDisplayedPercent(PROTECTION protection) {
    cancelAnimation();
    if (!useAnimation_ || displayedPercent_ == percent_) {
        displayedPercent_ = percent_;
        ui_.markDirty();
        return;
    }

    if (!ui_.animate(displayedPercent_, percent_, ANIMATION_DURATION_MS,
                     EasingType::EASE_OUT_CUBIC,
                     protection, &animation_)) {
        displayedPercent_ = percent_;
        animation_ = INVALID_ANIMATION_HANDLE;
        ui_.markDirty();
    }
}

/** @brief ProgressBar::drawSelf. */
void ProgressBar::drawSelf(const WidgetRenderContext& context) {
    const FocusBox bounds = getLocalBounds();
    if (bounds.w <= 0 || bounds.h <= 0) return;

    Canvas& canvas = ui_.getCanvas();
    const int32_t x = context.originX + bounds.x;
    const int32_t y = context.originY + bounds.y;
    canvas.drawRBox(x, y, bounds.w, bounds.h, 1);

    if (bounds.w <= 2 || bounds.h <= 2) return;

    const uint8_t drawColor = canvas.getDrawColor();
    const int32_t innerWidth = bounds.w - 2;
    const int32_t innerHeight = bounds.h - 2;
    canvas.setDrawColor(0);
    canvas.drawBox(x + 1, y + 1, innerWidth, innerHeight);
    canvas.setDrawColor(drawColor);

    const int32_t fillWidth = static_cast<int32_t>(
        (static_cast<int64_t>(innerWidth) * displayedPercent_) / MAX_PERCENT);
    if (fillWidth > 0) {
        canvas.drawBox(x + 1, y + 1, fillWidth, innerHeight);
    }
}

/** @brief ProgressBar::display. */
Canvas& ProgressBar::display() {
    return ui_.getCanvas();
}

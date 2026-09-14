#include "widgets/bitmap/bitmap_widget.h"

#include "PixelUI.h"

BitmapWidget::BitmapWidget(
    PixelUI& ui,
    int16_t x,
    int16_t y,
    uint16_t width,
    uint16_t height,
    const unsigned char* source)
    : ui_(ui), source_(source) {
    setWidgetBounds({x, y, width, height});
}

/** @brief BitmapWidget::setPosition. */
void BitmapWidget::setPosition(int16_t x, int16_t y) {
    const FocusBox bounds = getLocalBounds();
    setWidgetBounds({x, y, bounds.w, bounds.h});
}

/** @brief BitmapWidget::setSize. */
void BitmapWidget::setSize(uint16_t width, uint16_t height) {
    const FocusBox bounds = getLocalBounds();
    setWidgetBounds({bounds.x, bounds.y, width, height});
}

/** @brief BitmapWidget::drawSelf. */
void BitmapWidget::drawSelf(const WidgetRenderContext& context) {
    if (!source_) return;

    const FocusBox bounds = getLocalBounds();
    ui_.getCanvas().drawXBMP(
        context.originX + bounds.x,
        context.originY + bounds.y,
        bounds.w,
        bounds.h,
        source_);
}

/** @brief BitmapWidget::display. */
Canvas& BitmapWidget::display() { return ui_.getCanvas(); }

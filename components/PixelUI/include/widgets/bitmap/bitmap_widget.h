#pragma once

#include "widgets/IWidget.h"

class BitmapWidget : public IWidget {
public:
/** @brief BitmapWidget. */
    BitmapWidget(
        PixelUI& ui,
        int16_t x,
        int16_t y,
        uint16_t width,
        uint16_t height,
        const unsigned char* source = nullptr);

/** @brief onLoad. */
    void onLoad() override {}
/** @brief onOffload. */
    void onOffload() override {}

/** @brief setPosition. */
    void setPosition(int16_t x, int16_t y);
/** @brief setSize. */
    void setSize(uint16_t width, uint16_t height);
/** @brief setSource. */
    void setSource(const unsigned char* source) { source_ = source; }

private:
    PixelUI& ui_;
    const unsigned char* source_ = nullptr;

/** @brief drawSelf. */
    void drawSelf(const WidgetRenderContext& context) override;
/** @brief display. */
    Canvas& display() override;
};

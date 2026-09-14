#include "PixelUI.h"
#include "core/app/IApplication.h"
#include "core/app/app_system.h"
#include "focus/focus.h"
#include "widgets/label/label.h"
#include "widgets/icon_button/icon_button.h"
#include "widgets/text_button/text_button.h"
#include <cstring>
#include <new>

namespace {

constexpr uint8_t KEY_COUNT = 9;
constexpr uint8_t KEY_COLUMNS = 3;
constexpr uint16_t KEY_WIDTH = 34;
constexpr uint16_t KEY_HEIGHT = 12;
constexpr uint16_t KEY_X = 13;
constexpr uint16_t KEY_Y = 28;
constexpr uint32_t MULTI_TAP_TIMEOUT_MS = 800U;
static constexpr const char* KEY_LABELS[KEY_COUNT] = {
    "1,@.", "2!ABC", "3@DEF", "4#GHI", "5$JKL", "6%MNO",
    "7^PQRS", "8&TUV", "9*WXYZ"
};
static constexpr const char* KEY_CHARS[KEY_COUNT] = {
    "1,@.", "2!ABCabc", "3@DEFdef", "4#GHIghi", "5$JKLjkl", "6%MNOmno",
    "7^PQRSpqrs", "8&TUVtuv", "9*WXYZwxyz"
};

}

class TinyTextButton final : public TextButton {
    PixelUI& ui_;
    uint16_t x_;
    uint16_t y_;
    uint16_t width_;
    uint16_t height_;
    const char* text_;

    Canvas& display() override { return ui_.getCanvas(); }
    void drawSelf(const WidgetRenderContext& context) override {
        Canvas& canvas = ui_.getCanvas();
        const int32_t x = context.originX + x_;
        const int32_t y = context.originY + y_;
        canvas.drawRFrame(x, y, width_, height_, 1);
        canvas.setFont(PIXELUI_FONT_TINY);
        const int32_t textWidth = canvas.getUTF8Width(text_);
        const int32_t textHeight = canvas.getAscent() - canvas.getDescent();
        canvas.drawUTF8(x + (width_ - textWidth) / 2,
                        y + (height_ - textHeight) / 2 + canvas.getAscent(), text_);
    }

public:
    TinyTextButton(PixelUI& ui, uint16_t x, uint16_t y, uint16_t width,
                   uint16_t height, const char* text)
        : TextButton(ui, x, y, width, height, text), ui_(ui), x_(x), y_(y),
          width_(width), height_(height), text_(text) {}
};

class KeyboardFocusRoot final : public IWidget {
    PixelUI& ui_;
    void drawSelf(const WidgetRenderContext&) override {}
    Canvas& display() override { return ui_.getCanvas(); }
public:
    explicit KeyboardFocusRoot(PixelUI& ui) : ui_(ui) {}
    void onLoad() override {}
    void onOffload() override {}
};

class KeyboardTest final : public IApplication {
    PixelUI& ui_;
    FocusManager focus_;
    KeyboardFocusRoot focusRoot_;
    TextButton backspace_;
    char text_[25] = {};
    uint8_t length_ = 0;
    int8_t lastKey_ = -1;
    uint8_t lastCharIndex_ = 0;
    uint32_t lastPressTime_ = 0;
    Label value_;
    alignas(TinyTextButton) uint8_t storage_[KEY_COUNT][sizeof(TinyTextButton)];
    TinyTextButton* keys_[KEY_COUNT] = {};

    void append(char c) {
        if (length_ >= sizeof(text_) - 1U) return;
        text_[length_++] = c;
        text_[length_] = '\0';
        value_.setText(text_);
        ui_.markDirty();
    }
    void pressKey(uint8_t index) {
        const char* chars = KEY_CHARS[index];
        const uint8_t count = static_cast<uint8_t>(strlen(chars));
        const uint32_t now = ui_.getCurrentTime();
        const bool continuing = lastKey_ == static_cast<int8_t>(index) &&
                                now - lastPressTime_ <= MULTI_TAP_TIMEOUT_MS;
        uint8_t charIndex = continuing ? static_cast<uint8_t>((lastCharIndex_ + 1U) % count) : 0U;
        const char value = chars[charIndex];
        if (continuing && length_ != 0U) text_[length_ - 1U] = value;
        else append(value);
        lastKey_ = static_cast<int8_t>(index);
        lastCharIndex_ = charIndex;
        lastPressTime_ = now;
        value_.setText(text_);
        ui_.markDirty();
    }
    void erase() {
        if (length_ == 0U) return;
        text_[--length_] = '\0';
        lastKey_ = -1;
        lastPressTime_ = 0;
        value_.setText(text_);
        ui_.markDirty();
    }
    void configureKey(uint8_t index) {
        TextButton& button = *keys_[index];
        button.setCallback([this, index]() { pressKey(index); });
        button.onLoadNoAnim();
        button.setFocusable(true);
    }

public:
    explicit KeyboardTest(PixelUI& ui, void*)
        : ui_(ui), focus_(ui), focusRoot_(ui),
          backspace_(ui, 95, 12, 30, 12, "RETN", PIXELUI_FONT_SMALL),
          value_(ui, 2, 18, 106, text_, POS::BOTTOM, PIXELUI_FONT_SMALL) {
        backspace_.setCallback([this]() { erase(); });
        backspace_.onLoadNoAnim();
        focusRoot_.addChild(backspace_);
        for (uint8_t i = 0; i < KEY_COUNT; ++i) {
            const uint8_t row = i / KEY_COLUMNS;
            const uint8_t column = i % KEY_COLUMNS;
            keys_[i] = new (storage_[i]) TinyTextButton(ui_, KEY_X + column * KEY_WIDTH,
                KEY_Y + row * KEY_HEIGHT, KEY_WIDTH, KEY_HEIGHT, KEY_LABELS[i]);
            focusRoot_.addChild(*keys_[i]);
        }
    }
    ~KeyboardTest() override {
        for (uint8_t i = 0; i < KEY_COUNT; ++i) keys_[i]->~TinyTextButton();
    }
    void onEnter(ExitCallback cb) override {
        IApplication::onEnter(cb);
        value_.onLoadImmediately();
        focus_.addWidget(&focusRoot_);
        for (uint8_t i = 0; i < KEY_COUNT; ++i) configureKey(i);
        ui_.markDirty();
    }
    void draw() override {
        value_.draw();
        for (TinyTextButton* button : keys_) button->draw();
        backspace_.draw();
        focus_.draw();
    }
    bool handleInput(InputEvent event) override {
        if (event == InputEvent::BACK) {
            requestExit();
            return true;
        }
        return focus_.handleInput(event);
    }
    void onExit() override {
        focus_.clear();
        ui_.setContinousDraw(false);
        ui_.markFading();
    }
};

AppItem keyboard_test_app{
    .title = "键盘",
    .bitmap = nullptr,
    .factory = ApplicationFactory::make<KeyboardTest>(),
};

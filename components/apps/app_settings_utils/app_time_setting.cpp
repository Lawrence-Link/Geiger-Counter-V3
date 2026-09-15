/*
 * Copyright (C) 2025 Lawrence Link
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "core/app/IApplication.h"
#include "PixelUI.h"
#include "core/app/app_system.h"
#include "widgets/num_scroll/num_scroll.h"
#include "widgets/text_button/text_button.h"
#include "widgets/label/label.h"
#include "data_storage_core.h"
#include "time_module.h"
#include "i2c.h"

#include <cstring>

namespace {
constexpr IntegerFormat kTwoDigitFormat{2U, nullptr};
constexpr IntegerFormat kFourDigitFormat{4U, nullptr};

int32_t weekdayForDate(int32_t year, int32_t month, int32_t day) {
    static constexpr int32_t offsets[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    if (month < 3) --year;
    return (year + year / 4 - year / 100 + year / 400 +
            offsets[month - 1] + day) % 7;
}
}

class TimeSetting : public IApplication {
private:
    class FocusGroup : public IWidget {
    public:
        explicit FocusGroup(PixelUI& ui) : ui_(ui) { setFocusable(false); }
        void onLoad() override {}
        void onOffload() override {}
    private:
        PixelUI& ui_;
        void drawSelf(const WidgetRenderContext&) override {}
        Canvas& display() override { return ui_.getCanvas(); }
    } focusGroup_;
    PixelUI& ui_;
    NumScroll year_;
    NumScroll month_;
    NumScroll day_;
    NumScroll hour_;
    NumScroll minute_;
    NumScroll second_;
    TextButton writeButton_;
    Label title_;
    tm adjusted_{};
    bool rtcValid_ = false;
    bool written_ = false;
    int32_t dayMaximum_ = 0;

    void updateDayRange() {
        const int32_t maximum = DataStorageCore::daysInMonth(year_.getValue(), month_.getValue());
        if (maximum == dayMaximum_) return;
        dayMaximum_ = maximum;
        day_.setRange(1, maximum);
        if (day_.getValue() > maximum) day_.setValue(maximum);
    }

    void writeRtc() {
        updateDayRange();
        adjusted_.tm_year = year_.getValue() - 1900;
        adjusted_.tm_mon = month_.getValue() - 1;
        adjusted_.tm_mday = day_.getValue();
        adjusted_.tm_wday = weekdayForDate(year_.getValue(), month_.getValue(), day_.getValue());
        adjusted_.tm_hour = hour_.getValue();
        adjusted_.tm_min = minute_.getValue();
        adjusted_.tm_sec = second_.getValue();
        if (pcf8563_set_time(pcf8563_dev, &adjusted_) == ESP_OK) {
            written_ = true;
            writeButton_.setText("成功/返回");
        } else {
            ui_.showPopupInfo("RTC写入失败", "RTC", 82, 30, 2500);
        }
    }

public:
    TimeSetting(PixelUI& ui, void*)
        : IApplication(false), focusGroup_(ui), ui_(ui),
          year_(ui, 1, 20, 38, 15), month_(ui, 42, 20, 18, 15),
          day_(ui, 63, 20, 18, 15), hour_(ui, 1, 38, 24, 15),
          minute_(ui, 28, 38, 24, 15), second_(ui, 55, 38, 24, 15),
          writeButton_(ui, 84, 38, 42, 15, "写入", PIXELUI_FONT_TEXT, 0, -1),
          title_(ui, 3, 12, "RTC 日期和时间", POS::LEFT) {
        focusGroup_.addChild(year_);
        focusGroup_.addChild(month_);
        focusGroup_.addChild(day_);
        focusGroup_.addChild(hour_);
        focusGroup_.addChild(minute_);
        focusGroup_.addChild(second_);
        focusGroup_.addChild(writeButton_);
    }

    void draw() override {
        updateDayRange();
        Canvas& canvas = ui_.getCanvas();
        canvas.setFont(PIXELUI_FONT_TINY);
        canvas.drawStr(84, 25, "Y-M-D");
        canvas.drawStr(84, 33, "H:M:S");
        canvas.drawHLine(0, 15, 128);
        year_.draw();
        month_.draw();
        day_.draw();
        hour_.draw();
        minute_.draw();
        second_.draw();
        writeButton_.draw();
        title_.draw();
    }

    bool handleInput(InputEvent event) override {
        if (event == InputEvent::BACK) {
            requestExit();
            return true;
        }
        return false;
    }

    void onEnter(ExitCallback callback) override {
        IApplication::onEnter(callback);
        ui_.setContinousDraw(true);
        written_ = false;
        dayMaximum_ = 0;
        writeButton_.setText("写入");

        year_.setRange(2000, 2099);
        month_.setRange(1, 12);
        day_.setRange(1, 31);
        hour_.setRange(0, 23);
        minute_.setRange(0, 59);
        second_.setRange(0, 59);
        year_.setFormatter(NumericFormatter::integer(kFourDigitFormat));
        month_.setFormatter(NumericFormatter::integer(kTwoDigitFormat));
        day_.setFormatter(NumericFormatter::integer(kTwoDigitFormat));
        hour_.setFormatter(NumericFormatter::integer(kTwoDigitFormat));
        minute_.setFormatter(NumericFormatter::integer(kTwoDigitFormat));
        second_.setFormatter(NumericFormatter::integer(kTwoDigitFormat));

        std::memset(&adjusted_, 0, sizeof(adjusted_));
        if (pcf8563_get_time(pcf8563_dev, &adjusted_, &rtcValid_) != ESP_OK || !rtcValid_) {
            adjusted_.tm_year = 126;
            adjusted_.tm_mon = 0;
            adjusted_.tm_mday = 1;
        }
        year_.setValue(adjusted_.tm_year + 1900);
        month_.setValue(adjusted_.tm_mon + 1);
        day_.setValue(adjusted_.tm_mday);
        hour_.setValue(adjusted_.tm_hour);
        minute_.setValue(adjusted_.tm_min);
        second_.setValue(adjusted_.tm_sec);

        writeButton_.setCallback([this]() {
            if (written_) requestExit();
            else writeRtc();
        });

        title_.onLoad();
        year_.onLoad();
        month_.onLoad();
        day_.onLoad();
        hour_.onLoad();
        minute_.onLoad();
        second_.onLoad();
        writeButton_.onLoad();

        ui_.addWidgetToFocusManager(&focusGroup_);
        ui_.markDirty();
    }

    void onResume() override { ui_.setContinousDraw(true); }

    void onExit() override {
        ui_.clearAllAnimations();
        ui_.setContinousDraw(false);
        ui_.markFading();
    }
};

AppItem time_setting_app{
    .title = nullptr,
    .bitmap = nullptr,
    .factory = ApplicationFactory::make<TimeSetting>(),
};

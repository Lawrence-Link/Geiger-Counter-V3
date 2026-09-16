/*
 * Copyright (C) 2025 Lawrence Link
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "core/app/IApplication.h"
#include "PixelUI.h"
#include "core/app/app_system.h"
#include "core/coroutine/Coroutine.h"
#include "widgets/num_scroll/num_scroll.h"
#include "widgets/analog_clock/analog_clock.h"
#include "widgets/text_button/text_button.h"
#include "widgets/label/label.h"
#include "data_storage_core.h"
#include "time_module.h"
#include "i2c.h"

#include <cstdio>
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
    enum class Page : uint8_t {
        Date,
        Time,
    };

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
    Clock clock_;
    TextButton nextButton_;
    TextButton writeButton_;
    Label title_;
    tm adjusted_{};
    tm realtime_{};
    bool rtcValid_ = false;
    bool written_ = false;
    int32_t dayMaximum_ = 0;
    int32_t titleBarWidth_ = 0;
    uint32_t lastRtcRead_ = 0U;
    Page page_ = Page::Date;
    Coroutine dateLoad_;
    Coroutine timeLoad_;

    void loadPage(Page page) {
        ui_.removeCoroutine(&dateLoad_);
        ui_.removeCoroutine(&timeLoad_);
        ui_.clearAllAnimations();
        titleBarWidth_ = 0;
        page_ = page;
        const bool showDate = page_ == Page::Date;
        year_.setVisible(false);
        month_.setVisible(false);
        day_.setVisible(false);
        nextButton_.setVisible(false);
        hour_.setVisible(false);
        minute_.setVisible(false);
        second_.setVisible(false);
        writeButton_.setVisible(false);
        clock_.setVisible(false);
        title_.setVisible(false);
        title_.onOffload();
        title_.setText(showDate ? "RTC日期" : "RTC时间");
        Coroutine& loader = showDate ? dateLoad_ : timeLoad_;
        loader.reset();
        loader.start();
        ui_.addCoroutine(&loader);
        ui_.markDirty();
    }

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
            realtime_ = adjusted_;
            lastRtcRead_ = ui_.getCurrentTime();
            writeButton_.setText("成功");
        } else {
            ui_.showPopupInfo("RTC写入失败", "RTC", 82, 30, 2500);
        }
    }

public:
    TimeSetting(PixelUI& ui, void*)
        : IApplication(false), focusGroup_(ui), ui_(ui),
          year_(ui, 1, 25, 44, 16), month_(ui, 48, 25, 31, 16),
          day_(ui, 82, 25, 31, 16), hour_(ui, 1, 25, 24, 16),
          minute_(ui, 27, 25, 24, 16), second_(ui, 53, 25, 24, 16),
          clock_(ui, 103, 32, 20),
          nextButton_(ui, 1, 44, 112, 17, "下一页：时间", PIXELUI_FONT_TEXT, 0, -1),
          writeButton_(ui, 1, 44, 76, 17, "写入", PIXELUI_FONT_TEXT, 0, -1),
          title_(ui, 3, 14, "RTC日期", POS::LEFT),
          dateLoad_([this](CoroutineContext& context) {
              CORO_BEGIN(context);
              CORO_DELAY(context, ui_, 100, 1);
                  year_.setVisible(true);
                  year_.onLoad();
              CORO_DELAY(context, ui_, 100, 2);
                  title_.setVisible(true);
                  month_.setVisible(true);
                  title_.onLoad();
                  month_.onLoad();
                  if (!ui_.animate(titleBarWidth_, 78, 700,
                                   EasingType::EASE_IN_OUT_CUBIC,
                                   PROTECTION::PROTECTED)) {
                      titleBarWidth_ = 78;
                  }
              CORO_DELAY(context, ui_, 100, 3);
                  day_.setVisible(true);
                  day_.onLoad();
              CORO_DELAY(context, ui_, 100, 4);
                  nextButton_.setVisible(true);
                  nextButton_.onLoad();
              CORO_END(context);
          }),
          timeLoad_([this](CoroutineContext& context) {
              CORO_BEGIN(context);
              CORO_DELAY(context, ui_, 100, 1);
                  clock_.setVisible(true);
                  hour_.setVisible(true);
                  clock_.onLoad();
                  hour_.onLoad();
              CORO_DELAY(context, ui_, 100, 2);
                  title_.setVisible(true);
                  minute_.setVisible(true);
                  title_.onLoad();
                  minute_.onLoad();
                  if (!ui_.animate(titleBarWidth_, 78, 700,
                                   EasingType::EASE_IN_OUT_CUBIC,
                                   PROTECTION::PROTECTED)) {
                      titleBarWidth_ = 78;
                  }
              CORO_DELAY(context, ui_, 100, 3);
                  second_.setVisible(true);
                  second_.onLoad();
              CORO_DELAY(context, ui_, 100, 4);
                  writeButton_.setVisible(true);
                  writeButton_.onLoad();
              CORO_END(context);
          }) {
        focusGroup_.addChild(year_);
        focusGroup_.addChild(month_);
        focusGroup_.addChild(day_);
        focusGroup_.addChild(nextButton_);
        focusGroup_.addChild(hour_);
        focusGroup_.addChild(minute_);
        focusGroup_.addChild(second_);
        focusGroup_.addChild(writeButton_);
    }

    void draw() override {
        updateDayRange();
        Canvas& canvas = ui_.getCanvas();
        canvas.drawHLine(0, 19, titleBarWidth_);

        if (page_ == Page::Date) {
            year_.draw();
            month_.draw();
            day_.draw();
            nextButton_.draw();
        } else {
            const uint32_t now = ui_.getCurrentTime();
            if (written_ && now - lastRtcRead_ >= 1000U) {
                lastRtcRead_ = now;
                pcf8563_get_time(pcf8563_dev, &realtime_, &rtcValid_);
            }

            const tm& clockTime = written_ ? realtime_ : adjusted_;
            const uint8_t hour = written_
                ? static_cast<uint8_t>(clockTime.tm_hour)
                : static_cast<uint8_t>(hour_.getValue());
            const uint8_t minute = written_
                ? static_cast<uint8_t>(clockTime.tm_min)
                : static_cast<uint8_t>(minute_.getValue());
            const uint8_t second = written_
                ? static_cast<uint8_t>(clockTime.tm_sec)
                : static_cast<uint8_t>(second_.getValue());
            clock_.setHour(hour);
            clock_.setMinute(minute);
            clock_.setSecond(second);

            char currentTime[12];
            std::snprintf(currentTime, sizeof(currentTime), "%02u:%02u:%02u",
                          hour, minute, second);
            canvas.setFont(PIXELUI_FONT_SMALL);
            canvas.drawStr(84, 62, currentTime);
            hour_.draw();
            minute_.draw();
            second_.draw();
            clock_.draw();
            writeButton_.draw();
        }
        title_.draw();
    }

    bool handleInput(InputEvent event) override {
        if (event == InputEvent::BACK) {
            if (page_ == Page::Time) {
                loadPage(Page::Date);
                return true;
            }
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
        realtime_ = adjusted_;
        year_.setValue(adjusted_.tm_year + 1900);
        month_.setValue(adjusted_.tm_mon + 1);
        day_.setValue(adjusted_.tm_mday);
        hour_.setValue(adjusted_.tm_hour);
        minute_.setValue(adjusted_.tm_min);
        second_.setValue(adjusted_.tm_sec);

        nextButton_.setCallback([this]() { loadPage(Page::Time); });
        writeButton_.setCallback([this]() {
            if (written_) requestExit();
            else writeRtc();
        });

        ui_.addWidgetToFocusManager(&focusGroup_);
        loadPage(Page::Date);
    }

    void onResume() override { ui_.setContinousDraw(true); }

    void onExit() override {
        ui_.removeCoroutine(&dateLoad_);
        ui_.removeCoroutine(&timeLoad_);
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

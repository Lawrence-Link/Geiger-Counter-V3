/*
 * Copyright (C) 2025 Lawrence Link
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "PixelUI.h"
#include "core/CommonTypes.h"
#include "core/app/app_system.h"
#include "widgets/ChartSeries.h"
#include "widgets/histogram/histogram.h"
#include "widgets/icon_button/icon_button.h"
#include "widgets/text_button/text_button.h"
#include "widgets/progress_bar/progress_bar.h"
#include "calc/TextAlignHelper/TextAlignHelper.h"
#include "data_storage.h"
#include "data_storage_core.h"

#include <cstdio>
#include <cstring>

__attribute__((aligned(4)))
static const unsigned char image_storyboard_bits[] U8X8_PROGMEM = {
    0xf0,0xff,0x0f,0xfc,0xff,0x3f,0xfe,0xff,0x7f,0xfe,0xff,0x7f,
    0xff,0xff,0xff,0x1f,0xf8,0xff,0x1f,0xe0,0xff,0x1f,0xe0,0xff,
    0x0f,0x00,0xf0,0x0f,0x00,0xf0,0x0f,0x80,0xf1,0x0f,0x60,0xf0,
    0x0f,0x98,0xf1,0x0f,0x6c,0xf0,0x0f,0x06,0xf0,0x8f,0x01,0xf0,
    0x8f,0xff,0xf3,0x0f,0x00,0xf0,0x0f,0x00,0xf0,0xff,0xff,0xff,
    0xfe,0xff,0x7f,0xfe,0xff,0x7f,0xfc,0xff,0x3f,0xf0,0xff,0x0f
};
static const uint8_t image_ButtonLeftSmall_bits[] = {0x04,0x06,0x07,0x06,0x04};
static const uint8_t image_ButtonRightSmall_bits[] = {0x01,0x03,0x07,0x03,0x01};

namespace {
constexpr size_t kChartWidth = 124U;

class BrowseHistogram : public Histogram {
public:
    template <size_t N, uint16_t W, uint16_t H>
    BrowseHistogram(PixelUI& ui, uint16_t x, uint16_t y, uint16_t width,
                    uint16_t height, StaticChartSeries<N>& series,
                    ChartExpandSize<W, H> expanded, EXPAND_BASE base)
        : Histogram(ui, x, y, width, height, series, expanded, base) {
        setFocusable(true);
    }

    bool onSelect() override {
        Histogram::onSelect();
        setFocusable(false);
        // Expansion is a view mode, not an input capture. This lets the
        // focus cursor disappear and the encoder move to the timeline.
        return false;
    }

    void closeExpandedView() {
        if (!isExpanded()) return;
        Histogram::onSelect();
        setFocusable(true);
    }
};

class TimelineBar : public ProgressBar {
public:
    TimelineBar(PixelUI& ui, int16_t x, int16_t y, uint16_t width, uint16_t height)
        : ProgressBar(ui, x, y, width, height, 100, false), ui_(ui) {
        setFocusable(true);
    }

    void setChangedCallback(VoidCallback callback) { changed_ = callback; }
    bool onSelect() override { return true; }
    bool handleEvent(InputEvent event) override {
        if (event == InputEvent::SELECT) return true;
        int32_t next = getPercent();
        if (event == InputEvent::LEFT) next -= 5;
        else if (event == InputEvent::RIGHT) next += 5;
        else return false;
        setPercentImmediate(next);
        updateInteractionTime(ui_.getCurrentTime());
        if (changed_) changed_();
        return false;
    }
private:
    PixelUI& ui_;
    VoidCallback changed_;
};
}

class Storyboard : public IApplication {
private:
    PixelUI& ui_;
    StaticChartSeries<kChartWidth> series_;
    BrowseHistogram histogram_;
    IconButton previous_;
    IconButton next_;
    TextButton delete_;
    ProgressBar usage_;
    TimelineBar timeline_;

    uint32_t fileId_ = 0U;
    size_t recordCount_ = 0U;
    StorageWindowInfo window_{};
    char fileLabel_[16] = "0.csv";
    char usageLabel_[24] = "Used: --%";

    void updateUsage() {
        StorageStats stats;
        int32_t percent = 0;
        if (data_storage_get_stats(stats) && stats.totalBytes != 0U) {
            percent = static_cast<int32_t>((stats.usedBytes * 100U) / stats.totalBytes);
            std::snprintf(usageLabel_, sizeof(usageLabel_), "Used: %ld%%",
                          static_cast<long>(percent));
        } else {
            std::snprintf(usageLabel_, sizeof(usageLabel_), "Used: --%%");
        }
        usage_.setPercentImmediate(percent);
    }

    void loadCurrentWindow() {
        series_.clear();
        window_ = {};
        if (fileId_ == 0U) {
            std::snprintf(fileLabel_, sizeof(fileLabel_), "no csv");
            recordCount_ = 0U;
            return;
        }
        std::snprintf(fileLabel_, sizeof(fileLabel_), "%lu.csv",
                      static_cast<unsigned long>(fileId_));
        recordCount_ = data_storage_count_records(fileId_);
        const size_t start = DataStorageCore::windowStartForPercent(
            recordCount_, kChartWidth, timeline_.getPercent());
        int32_t values[kChartWidth]{};
        if (!data_storage_load_window(fileId_, start, values, kChartWidth, window_)) return;
        for (size_t index = 0U; index < window_.loadedRecords; ++index) series_.add(values[index]);
        ui_.markDirty();
    }

    void selectPrevious() {
        const uint32_t candidate = data_storage_previous_file(fileId_);
        if (candidate != fileId_) {
            fileId_ = candidate;
            timeline_.setPercentImmediate(100);
            loadCurrentWindow();
        }
    }

    void selectNext() {
        const uint32_t candidate = data_storage_next_file(fileId_);
        if (candidate != fileId_) {
            fileId_ = candidate;
            timeline_.setPercentImmediate(100);
            loadCurrentWindow();
        }
    }

    void deleteCurrent() {
        if (fileId_ == 0U || !data_storage_delete_file(fileId_)) return;
        fileId_ = data_storage_latest_file();
        timeline_.setPercentImmediate(100);
        updateUsage();
        loadCurrentWindow();
    }

    void drawRange(Canvas& canvas) {
        canvas.setFont(PIXELUI_FONT_TINY);
        if (window_.loadedRecords == 0U) {
            canvas.drawStr(2, 56, "No recorded data");
            return;
        }
        char range[40];
        std::snprintf(range, sizeof(range), "%5s %5.5s - %5s %5.5s",
                      window_.firstDate + 5, window_.firstTime,
                      window_.lastDate + 5, window_.lastTime);
        canvas.drawStr(2, 56, range);
    }

public:
    Storyboard(PixelUI& ui, void*)
        : ui_(ui),
          histogram_(ui, 2, 0, 62, 37, series_, ChartExpandSize<124, 46>{},
                     EXPAND_BASE::TOP_LEFT),
          previous_(ui, 3, 42, 3, 5, image_ButtonLeftSmall_bits),
          next_(ui, 36, 42, 3, 5, image_ButtonRightSmall_bits),
          delete_(ui, 43, 40, 21, 9, "DEL", PIXELUI_FONT_TINY, 0, -1),
          usage_(ui, 2, 60, 63, 4, 0, false),
          timeline_(ui, 2, 60, 124, 4) {
        setEnterTransitionEnabled(true);
        timeline_.setVisible(false);
    }

    void draw() override {
        Canvas& canvas = ui_.getCanvas();
        const bool expanded = histogram_.isExpanded();
        previous_.setVisible(!expanded);
        next_.setVisible(!expanded);
        delete_.setVisible(!expanded);
        usage_.setVisible(!expanded);
        timeline_.setVisible(expanded);

        if (expanded) {
            histogram_.draw();
            drawRange(canvas);
            timeline_.draw();
            return;
        }

        histogram_.draw();
        previous_.draw();
        next_.draw();
        delete_.draw();
        usage_.draw();
        canvas.setFont(PIXELUI_FONT_TINY);
        const int labelX = TextAlignHelper::calcCenteredX(
            canvas.rawDisplay().getU8g2(), 3, 37, fileLabel_);
        canvas.drawStr(labelX, 47, fileLabel_);
        canvas.setFont(PIXELUI_FONT_SMALL);
        canvas.drawStr(11, 58, usageLabel_);
        if (window_.loadedRecords != 0U) {
            canvas.setFont(u8g2_font_6x12_tr);
            canvas.drawStr(89, 9, "From");
            canvas.drawStr(96, 42, "To");

            canvas.setFont(PIXELUI_FONT_SMALL);
            canvas.drawStr(80, 18, window_.firstDate + 2);
            canvas.drawStr(80, 51, window_.lastDate + 2);

            canvas.setFont(u8g2_font_6x13B_tr);
            canvas.drawStr(77, 29, window_.firstTime);
            canvas.drawStr(77, 62, window_.lastTime);
        }
    }

    bool handleInput(InputEvent event) override {
        if (event == InputEvent::BACK) {
            if (histogram_.isExpanded()) {
                histogram_.closeExpandedView();
                ui_.markDirty();
                return true;
            }
            requestExit();
            return true;
        }
        return false;
    }

    void onEnter(ExitCallback callback) override {
        IApplication::onEnter(callback);
        histogram_.onLoad();
        previous_.onLoad();
        next_.onLoad();
        delete_.onLoad();
        usage_.onLoad();
        timeline_.onLoad();
        previous_.setCallback([this]() { selectPrevious(); });
        next_.setCallback([this]() { selectNext(); });
        delete_.setCallback([this]() { deleteCurrent(); });
        timeline_.setChangedCallback([this]() { loadCurrentWindow(); });
        ui_.addWidgetToFocusManager(&histogram_);
        ui_.addWidgetToFocusManager(&previous_);
        ui_.addWidgetToFocusManager(&next_);
        ui_.addWidgetToFocusManager(&delete_);
        ui_.addWidgetToFocusManager(&timeline_);
        fileId_ = data_storage_latest_file();
        timeline_.setPercentImmediate(100);
        updateUsage();
        loadCurrentWindow();
    }

    void onExit() override {
        ui_.clearAllAnimations();
        ui_.markFading();
    }
};

AppItem storyboard_app{
    .title = "历史存储",
    .bitmap = image_storyboard_bits,
    .factory = ApplicationFactory::make<Storyboard>(),
};

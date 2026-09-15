/*
 * Copyright (C) 2025 Lawrence Link
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
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

__attribute__((aligned(4)))
static const unsigned char image_storyboard_bits[] = {0xf0,0xff,0x0f,0xfc,0xff,0x3f,0xfe,0xff,0x7f,0xfe,0xff,0x77,0xff,0xc3,0xef,0xff,0x81,0xff,0xff,0x1c,0xff,0xff,0x3e,0xff,0xff,0x3f,0xff,0xff,0x3f,0xff,0xff,0x1f,0xff,0xff,0x9f,0xff,0xff,0x8f,0xff,0xff,0xc7,0xff,0xff,0xe7,0xff,0xff,0xe7,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xef,0xe7,0xff,0xdf,0xe7,0xff,0xfe,0xff,0x7f,0xfe,0xff,0x7f,0xfc,0xff,0x3f,0xf0,0xff,0x0f};
static const uint8_t image_ButtonLeftSmall_bits[] = {0x04,0x06,0x07,0x06,0x04};
static const uint8_t image_ButtonRightSmall_bits[] = {0x01,0x03,0x07,0x03,0x01};


class Storyboard: public IApplication {
private:
    PixelUI& m_ui;
    StaticChartSeries<62> m_series;
    Histogram m_histogram;
    IconButton btn_prev;
    IconButton btn_next; 
    TextButton btn_back;
    ProgressBar usagebar;
public:
    Storyboard(PixelUI& ui, void* param):
    m_ui(ui), 
    m_histogram(
        m_ui,
        5,                              // 左上角 X
        0,                              // 左上角 Y
        62,                              // 默认宽度
        37,                              // 默认高度
        m_series,                         // 数据序列
        ChartExpandSize<62, 37>{},        // 展开后的宽度和高度
        EXPAND_BASE::TOP_LEFT        // 展开时保持右下角不动
    ),
    btn_prev(m_ui, 6, 42, 3, 5, image_ButtonLeftSmall_bits),
    btn_next(m_ui, 39, 42, 3, 5, image_ButtonRightSmall_bits),
    btn_back(m_ui, 46, 40, 21, 9, "DEL", PIXELUI_FONT_TINY, 0, -1),
    usagebar(m_ui, 2, 60, 70, 4)
    {
        // m_series.setMaxValue(100);
        // m_series.setMinValue(0);
        // m_series.setColor(1);
        setEnterTransitionEnabled(true);
    }
    
    void draw() override {
        Canvas& canvas = m_ui.getCanvas();

        m_histogram.draw();
        btn_prev.draw();
        btn_next.draw();
        btn_back.draw();
        usagebar.draw();

        /// 
        char buf[] = "9/10";

        canvas.setFont(u8g2_font_4x6_tr);

        int calcX = TextAlignHelper::calcCenteredX(
            m_ui.getCanvas().rawDisplay().getU8g2(),
            6, 37, buf
        );

        canvas.drawStr(calcX, 47, buf);
        canvas.setFont(u8g2_font_5x7_tr);
        canvas.drawStr(11, 58, "Used: 60%");

        canvas.setFont(u8g2_font_5x7_tr);
        canvas.drawStr(80, 20, "26-SEP-14");
        canvas.drawStr(80, 51, "26-SEP-14");

        canvas.setFont(u8g2_font_6x13B_tr);
        canvas.drawStr(77, 31, "10:10:14");
        canvas.drawStr(77, 62, "10:10:38");

        canvas.setFont(u8g2_font_6x12_tr);
        canvas.drawStr(94, 43, "To");
        canvas.drawStr(89, 10, "From");

        ///
    }

    bool handleInput(InputEvent event) override {
        if (event == InputEvent::BACK) {
            requestExit();
            return true;
        }
        return false;
    }
    
    void onEnter(ExitCallback cb) override {
        IApplication::onEnter(cb);
        m_histogram.onLoad();
        btn_prev.onLoad();
        btn_next.onLoad();
        btn_back.onLoad();
        usagebar.onLoad();
        usagebar.setPercent(70);

        m_ui.addWidgetToFocusManager(&btn_prev);
        m_ui.addWidgetToFocusManager(&btn_next);
        m_ui.addWidgetToFocusManager(&btn_back);
    }

    void onExit() {
        m_ui.markFading();
    }
};

AppItem storyboard_app{
    .title = "历史存储",
    .bitmap = image_storyboard_bits,
    
    .factory = ApplicationFactory::make<Storyboard>()
};

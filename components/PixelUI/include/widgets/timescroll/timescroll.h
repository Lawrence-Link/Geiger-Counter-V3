#pragma once

#include "widgets/IWidget.h"

class timescroll : public IWidget {
private:
    PixelUI& m_ui;
public:
    timescroll(PixelUI& ui) : m_ui(ui) {}

    void onLoad() override {

    }
    void draw () override {
        U8G2 u8g2 = m_ui.getU8G2();
        
    }

};
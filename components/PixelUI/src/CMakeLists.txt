cmake_minimum_required(VERSION 3.16)
project(PixelUI LANGUAGES CXX)

set(U8G2_C_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/u8g2/csrc")
set(U8G2_CPP_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/u8g2/cppsrc")

add_library(pixelui STATIC
    PixelUI.cpp
    core/app/app_system.cpp
    core/animation/animation.cpp
    core/ViewManager/ViewManager.cpp
    core/coroutine/Coroutine.cpp
    
    ui/Popup/PopupBase.cpp
    ui/Popup/PopupInfo.cpp
    ui/Popup/PopupManager.cpp
    ui/Popup/PopupProgress.cpp
    ui/Popup/PopupValue4Digits.cpp
    
    ui/ListView/ListView.cpp
    ui/AppLauncher/AppLauncher.cpp
    ui/IconView/IconView.cpp
    
    widgets/histogram/histogram.cpp
    widgets/brace/brace.cpp
    widgets/icon_button/icon_button.cpp
    widgets/text_button/text_button.cpp
    widgets/num_scroll/num_scroll.cpp
    widgets/analog_clock/analog_clock.cpp
    widgets/label/label.cpp
    widgets/curve_chart/curve_chart.cpp

    focus/focus.cpp
    blinker/Blinker.cpp
    
)

target_include_directories(pixelui
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/etl/include
        ${U8G2_CPP_SRC_DIR}
        ${U8G2_C_SRC_DIR}
)

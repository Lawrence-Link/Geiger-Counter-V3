// Blinker.h
#pragma once
#include <cstdint>

class PixelUI;

class Blinker {
public:
    Blinker(PixelUI& ui, uint32_t interval_ms = 500);

    void start();              
    void stop();    
    void stopOnVisible(); 

    void set_interval(uint32_t interval_ms);

    void update();

    bool is_visible() const;
    bool is_running() const;

private:
    PixelUI&   m_ui;
    uint32_t   m_interval_ms;
    uint32_t   m_last_toggle_ms;
    bool       m_visible;
    bool       m_running;
    bool       m_stop_on_visible;
};

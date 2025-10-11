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

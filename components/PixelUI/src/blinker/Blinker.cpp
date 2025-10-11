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

#include "blinker/Blinker.h"
#include "PixelUI.h"
#include <cassert>

Blinker::Blinker(PixelUI& ui, uint32_t interval_ms)
    : m_ui(ui),
      m_interval_ms(interval_ms),
      m_last_toggle_ms(m_ui.getCurrentTime()),
      m_visible(true),
      m_running(false),
      m_stop_on_visible(false)
{
    if (m_interval_ms == 0) m_interval_ms = 500;
}

void Blinker::start() {
    if (!m_running) {
        m_running = true;
        m_last_toggle_ms = m_ui.getCurrentTime();
        m_stop_on_visible = false;
    }
}

void Blinker::stop() {
    m_running = false;
    m_visible = false;
    m_stop_on_visible = false;
}

void Blinker::stopOnVisible() {
    if (m_visible) {
        m_running = false;
        m_stop_on_visible = false;
        return;
    }
    m_stop_on_visible = true;
    m_running = true;
    m_last_toggle_ms = m_ui.getCurrentTime();
}

void Blinker::set_interval(uint32_t interval_ms) {
    m_interval_ms = (interval_ms == 0) ? 1 : interval_ms;
}

void Blinker::update() {
    if (!m_running) return;

    uint32_t now = m_ui.getCurrentTime();
    uint32_t delta = now - m_last_toggle_ms;

    if (m_interval_ms == 0) {
        m_visible = true;
        if (m_stop_on_visible && m_visible) {
            m_running = false;
            m_stop_on_visible = false;
        }
        return;
    }

    if (delta >= m_interval_ms) {
        uint32_t toggles = delta / m_interval_ms;
        if (toggles & 1u) {
            m_visible = !m_visible;
        }
        m_last_toggle_ms += toggles * m_interval_ms;

        if (m_stop_on_visible && m_visible) {
            m_running = false;
            m_stop_on_visible = false;
        }
    }
}

bool Blinker::is_visible() const {
    return m_visible;
}

bool Blinker::is_running() const {
    return m_running;
}

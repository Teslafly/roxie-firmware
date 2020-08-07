/*
    
    This file is part of the Roxie firmware.
    Roxie firmware is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    Roxie firmware is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with Roxie firmware.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "simple_vertical_screen.h"
#include "screen.h"
#include "util.h"
#include "tft_util.h"
#include <TFT_22_ILI9225.h>



void SimpleVerticalScreen::reset() {
    _tft->fillRectangle(0, 0, _tft->maxX() - 1, _tft->maxY() - 1, COLOR_BLACK);

        // labels
    _tft->setFont(Terminal6x8);
    _tft->drawText(0, 130, _config->imperial_units ? "TRIP MI" : "TRIP KM", COLOR_WHITE);
    _tft->drawText(0, 180, _config->imperial_units ? "TOTAL MI" : "TOTAL KM", COLOR_WHITE);
    _tft->drawText(110, 130, "WATTS", COLOR_WHITE);
    _tft->drawText(110, 180, "BATTERY V", COLOR_WHITE);

    switch (_primary_item) {
        case SCR_BATTERY_CURRENT:
            _tft->drawText(82, 0, "BATTERY A", COLOR_WHITE);
            break;
        case SCR_MOTOR_CURRENT:
            _tft->drawText(96, 0, "MOTOR A", COLOR_WHITE);
            break;
        default:
            _tft->drawText(150, 21, _config->imperial_units ? "MPH" : "KPH", COLOR_WHITE);
    }

    // FW version
    // _tft->drawText(0, 18, _config->fw_version, COLOR_WHITE);

    _just_reset = true;
}

void SimpleVerticalScreen::update(t_data *data) {
    char fmt[10];

    if (data->vesc_fault_code != _last_fault_code)
        reset();

    // primary display item
    float value = primary_item_value(_primary_item, data, _config);
    uint16_t color = primary_item_color(_primary_item, data, _config);
    dtostrf(value, 4, 1, fmt);
    tft_util_draw_number(_tft, fmt, 0, 35, color, COLOR_BLACK, 10, 14);

    // trip distance
    dtostrf(convert_distance(data->trip_km, _config->imperial_units), 5, 2, fmt);
    tft_util_draw_number(_tft, fmt, 0, 140, progress_to_color(data->session_reset_progress, _tft), COLOR_BLACK, 2, 6);

    // total distance
    format_total_distance(convert_distance(data->total_km, _config->imperial_units), fmt);
    tft_util_draw_number(_tft, fmt, 0, 190, COLOR_WHITE, COLOR_BLACK, 2, 6);

    // watts
    dtostrf(data->battery_amps * data->voltage, 4, 0, fmt);
    tft_util_draw_number(_tft, fmt, 95, 140, progress_to_color(data->mah_reset_progress, _tft), COLOR_BLACK, 2, 6);

    // battery voltage
    if (_config->per_cell_voltage)
        dtostrf(data->voltage / _config->battery_cells, 4, 2, fmt);
    else
        dtostrf(data->voltage, 4, 1, fmt);
    tft_util_draw_number(_tft, fmt, 110, 190, COLOR_WHITE, COLOR_BLACK, 2, 6);

    // warning
    if (data->vesc_fault_code != FAULT_CODE_NONE) {
        uint16_t bg_color = _tft->setColor(150, 0, 0);
        _tft->fillRectangle(0, 180, 176, 220, bg_color);
        _tft->setFont(Terminal12x16);
        _tft->setBackgroundColor(bg_color);
        _tft->drawText(5, 193, vesc_fault_code_to_string(data->vesc_fault_code), COLOR_BLACK);
        _tft->setBackgroundColor(COLOR_BLACK);
    }

    _update_battery_indicator(data->battery_percent, _just_reset);

    _last_fault_code = data->vesc_fault_code;
    _just_reset = false;
}

void SimpleVerticalScreen::_update_battery_indicator(float battery_percent, bool redraw) {
    int width = 15;
    int space = 2;
    int cell_count = 10;

    int cells_to_fill = round(battery_percent * cell_count);
    for (int i=0; i<cell_count; i++) {
        bool is_filled = (i < _battery_cells_filled);
        bool should_be_filled = (i < cells_to_fill);
        if (should_be_filled != is_filled || redraw) {
            int x = (i) * (width + space);
            uint8_t green = (uint8_t)(255.0 / (cell_count - 1) * i);
            uint8_t red = 255 - green;
            uint16_t color = _tft->setColor(red, green, 0);
            _tft->fillRectangle(x + 4, 1, x + width + 3, 15, color);
            if (!should_be_filled)
                _tft->fillRectangle(x + 5, 2, x + width  + 2, 14, COLOR_BLACK);
        }
    }
    _battery_cells_filled = cells_to_fill;
}

#define HEARTBEAT_X ((176 - HEARTBEAT_SIZE)/2 + 9)
#define HEARBEAT_Y 220-HEARTBEAT_SIZE-1

void SimpleVerticalScreen::heartbeat(uint32_t duration_ms, bool successful_vesc_read) {
    uint16_t color = successful_vesc_read ? _tft->setColor(0, 150, 0) : _tft->setColor(150, 0, 0);
    _tft->fillRectangle(HEARTBEAT_X, HEARBEAT_Y, HEARTBEAT_X + HEARTBEAT_SIZE, HEARBEAT_Y + HEARTBEAT_SIZE, color);
    delay(duration_ms);
    _tft->fillRectangle(HEARTBEAT_X, HEARBEAT_Y, HEARTBEAT_X + HEARTBEAT_SIZE, HEARBEAT_Y + HEARTBEAT_SIZE, COLOR_BLACK);
}
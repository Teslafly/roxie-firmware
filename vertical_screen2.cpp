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

#include "vertical_screen2.h"
#include "screen.h"
#include "util.h"
#include "tft_util.h"
#include <TFT_22_ILI9225.h>
#include "eeprom_backup.h"


void VerticalScreen2::draw_basic() {
    _tft->fillRectangle(0, 0, 176, 220, COLOR_BLACK);

    _tft->setFont(Terminal6x8);
    _tft->drawText(0, 20, "MOSFET TEMP", COLOR_WHITE);
    _tft->drawText(0, 75, "MOTOR TEMP", COLOR_WHITE);
    _tft->drawText(105, 20, "MOTOR AMPS", COLOR_WHITE);
    _tft->drawText(105, 75,"BAT AMPS", COLOR_WHITE);
    _tft->drawText(0, 130, "MAH CHARGED", COLOR_WHITE);
    _tft->drawText(0, 180, "DUTY CYCLE", COLOR_WHITE);
    _tft->drawText(105, 130, "MAH USED", COLOR_WHITE);
    _tft->drawText(112, 180,  "WH SPENT", COLOR_WHITE);


    _just_reset = true;
}

void VerticalScreen2::update(t_data *data) {
    char primary_value[10];
    char value1[10];
    char value2[10];
    char value3[10];
    char value4[10];
    char value5[10];
    char value6[10];
    char value7[10];
    char value8[10];

    if (data->vesc_fault_code != _last_fault_code)
        draw_basic();


    dtostrf(data->mosfet_celsius, 5, 1, value1);
    dtostrf(data->motor_celsius, 5, 1, value2);
    dtostrf(data->motor_amps, 4, 1, value3);
    dtostrf(data->battery_amps, 4, 1, value4);
    dtostrf(data->mah_charged, 5, 1, value5);
    dtostrf(min(data->duty_cycle * 100, 99.9), 5, 1, value6);
    dtostrf(data->mah_discharged, 4, 1, value7);
    dtostrf(data->wh_spent, 4, 1, value8);

    tft_util_draw_number(_tft, value1, 0, 30, COLOR_WHITE, COLOR_BLACK, 2, 6);
    tft_util_draw_number(_tft, value2, 0, 85, COLOR_WHITE, COLOR_BLACK, 2, 6);
    tft_util_draw_number(_tft, value3, 110, 30, COLOR_WHITE, COLOR_BLACK, 2, 6);
    tft_util_draw_number(_tft, value4, 110, 85, COLOR_WHITE, COLOR_BLACK, 2, 6);

    tft_util_draw_number(_tft, value5, 0, 140, COLOR_WHITE, COLOR_BLACK, 2, 6);
    tft_util_draw_number(_tft, value6, 0, 190, COLOR_WHITE, COLOR_BLACK, 2, 6);
    tft_util_draw_number(_tft, value7, 110, 140, COLOR_WHITE, COLOR_BLACK, 2, 6);
    tft_util_draw_number(_tft, value8, 110, 190, COLOR_WHITE, COLOR_BLACK, 2, 6);
    

    // warning
    if (data->vesc_fault_code != FAULT_CODE_NONE) {
        uint16_t bg_color = _tft->setColor(150, 0, 0);
        _tft->fillRectangle(0, 180, 176, 220, bg_color);
        _tft->setFont(Terminal12x16);
        _tft->setBackgroundColor(bg_color);
        _tft->drawText(5, 193, vesc_fault_code_to_string(data->vesc_fault_code), COLOR_BLACK);
        _tft->setBackgroundColor(COLOR_BLACK);
    }

    _update_battery_indicator(data->voltage_percent, _just_reset);

    _last_fault_code = data->vesc_fault_code;
    _just_reset = false;
}

void VerticalScreen2::_update_battery_indicator(float battery_percent, bool redraw) {
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

void VerticalScreen2::heartbeat(uint32_t duration_ms, bool successful_vesc_read) {
    uint16_t color = successful_vesc_read ? _tft->setColor(0, 150, 0) : _tft->setColor(150, 0, 0);
    _tft->fillRectangle(HEARTBEAT_X, HEARBEAT_Y, HEARTBEAT_X + HEARTBEAT_SIZE, HEARBEAT_Y + HEARTBEAT_SIZE, color);
    delay(duration_ms);
    _tft->fillRectangle(HEARTBEAT_X, HEARBEAT_Y, HEARTBEAT_X + HEARTBEAT_SIZE, HEARBEAT_Y + HEARTBEAT_SIZE, COLOR_BLACK);
}

bool VerticalScreen2::process_buttons(t_data *data, bool long_click){
    // Reset session and also write this to the EEPROM
    data->session->trip_meters = 0;
    data->session->max_speed_kph = 0;
    data->session->millis_elapsed = 0;
    data->session->millis_riding = 0;
    // session_data.min_voltage = data.voltage;
    eeprom_write_session_data(*data->session);
    return false;
}
// Copyright 2026 splitkb.com (support@splitkb.com)
// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H

bool is_oled_enabled = true;

bool module_oled_housekeeping_task_kb(void) {
    is_oled_enabled = last_input_activity_elapsed() < HLC_BACKLIGHT_TIMEOUT;

    if (!is_oled_enabled) {
        oled_off();
        return false;
    } else {
        oled_on();
    }

    return false;
}

// Copyright 2026 StoicaTielemans
// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H

// Layers and encoders live in keymap.json, generated from the .vil save file
// by scripts/vil_to_firmware.py. This file only adds per-layer RGB lighting:
//   layer 0     typing heatmap: red at rest, shifting to blue as keys get hot
//   layer 1     solid white
//   layers 2-7  solid colour from layer_hues
// Brightness always comes from the current RGB settings (Vial's Lighting tab).

// Hue (0-255) of the heatmap's cold and hot ends. Going down from red (0 = 256)
// to blue (170) passes through magenta and purple.
#define HEATMAP_COLD_HUE 0
#define HEATMAP_HOT_HUE 170

static const uint8_t layer_hues[] = {
    [2] = 85,  // green
    [3] = 191, // purple
    [4] = 43,  // yellow
    [5] = 128, // cyan
    [6] = 213, // magenta
    [7] = 21,  // orange
};

static void set_lighting_mode(uint8_t mode, uint8_t hue, uint8_t sat) {
    // Restarting the same mode would wipe the heatmap
    if (rgb_matrix_get_mode() != mode) {
        rgb_matrix_mode_noeeprom(mode);
    }
    // noeeprom: switching layers shouldn't wear out the flash
    rgb_matrix_sethsv_noeeprom(hue, sat, rgb_matrix_get_val());
}

static void set_layer_lighting(layer_state_t layers) {
    uint8_t layer = get_highest_layer(layers);

    switch (layer) {
        case 0:
            set_lighting_mode(RGB_MATRIX_TYPING_HEATMAP, HEATMAP_COLD_HUE, 255);
            break;
        case 1:
            set_lighting_mode(RGB_MATRIX_SOLID_COLOR, 0, 0); // white
            break;
        default:
            if (layer < ARRAY_SIZE(layer_hues)) {
                set_lighting_mode(RGB_MATRIX_SOLID_COLOR, layer_hues[layer], 255);
            }
            break;
    }
}

void keyboard_post_init_user(void) {
    set_layer_lighting(layer_state | default_layer_state);
}

layer_state_t layer_state_set_user(layer_state_t state) {
    set_layer_lighting(state | default_layer_state);
    return state;
}

layer_state_t default_layer_state_set_user(layer_state_t state) {
    set_layer_lighting(layer_state | state);
    return state;
}

// QMK's heatmap tracks how hot each key is in g_rgb_frame_buffer, but turns cold
// keys and underglow off and goes from blue to red. Repaint it with our own colours after
// every frame. Runs on both halves, since each half renders its own LEDs.
bool rgb_matrix_indicators_advanced_user(uint8_t led_min, uint8_t led_max) {
    if (rgb_matrix_get_mode() != RGB_MATRIX_TYPING_HEATMAP) {
        return false;
    }

    uint8_t val = rgb_matrix_get_val();

    // Underglow LEDs aren't under a key, so they just show the cold colour
    rgb_t cold = hsv_to_rgb((hsv_t){HEATMAP_COLD_HUE, 255, val});
    for (uint8_t i = led_min; i < led_max; i++) {
        rgb_matrix_set_color(i, cold.r, cold.g, cold.b);
    }

    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        for (uint8_t col = 0; col < MATRIX_COLS; col++) {
            uint8_t led = g_led_config.matrix_co[row][col];
            if (led < led_min || led >= led_max) {
                continue;
            }
            uint8_t heat = g_rgb_frame_buffer[row][col];
            hsv_t   hsv  = {HEATMAP_COLD_HUE - heat * (HEATMAP_COLD_HUE + 256 - HEATMAP_HOT_HUE) / 256, 255, val};
            rgb_t   rgb  = hsv_to_rgb(hsv);
            rgb_matrix_set_color(led, rgb.r, rgb.g, rgb.b);
        }
    }
    return false;
}

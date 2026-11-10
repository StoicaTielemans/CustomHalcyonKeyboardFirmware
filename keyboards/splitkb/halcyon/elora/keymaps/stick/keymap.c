// Copyright 2026 StoicaTielemans
// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H

// Layers and encoders live in keymap.json, generated from the .vil save file
// by scripts/vil_to_firmware.py. This file only adds per-layer RGB lighting:
//   layer 0     typing heatmap: brick red at rest, through cream to steel blue as keys get hot
//   layer 1     typing heatmap: white smoke at rest, through dry sage to dusty olive as keys get hot
//   layers 2-7  solid colour from layer_hues
// Brightness always comes from the current RGB settings (Vial's Lighting tab).

// Heatmap gradients, cold to hot, evenly spaced. Colours are at full brightness
// and get scaled by the current RGB brightness.
typedef struct {
    const rgb_t *stops;
    uint8_t      count;
} heatmap_palette_t;

static const rgb_t heatmap_layer0[] = {
    {0xC1, 0x12, 0x1F}, // brick red (#c1121f)
    {0xFD, 0xF0, 0xD5}, // papaya whip (#fdf0d5)
    {0x66, 0x9B, 0xBC}, // steel blue (#669bbc)
};

static const rgb_t heatmap_layer1[] = {
    {0xEF, 0xF1, 0xED}, // white smoke (#eff1ed)
    {0xBC, 0xBD, 0x8B}, // dry sage (#bcbd8b)
    {0x71, 0x77, 0x44}, // dusty olive (#717744)
};

// Indexed by layer. The layer's index is passed to the other half as the RGB
// hue, since only the RGB settings (not the layer) are synced between halves.
static const heatmap_palette_t heatmap_palettes[] = {
    {heatmap_layer0, ARRAY_SIZE(heatmap_layer0)},
    {heatmap_layer1, ARRAY_SIZE(heatmap_layer1)},
};

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
        case 1:
            set_lighting_mode(RGB_MATRIX_TYPING_HEATMAP, layer, 255);
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

static uint8_t lerp8(uint8_t a, uint8_t b, uint16_t t, uint16_t span) {
    return a + ((int16_t)b - a) * (int32_t)t / span;
}

// Colour for a heat level (0-255) on a palette, scaled to brightness val
static rgb_t heatmap_color(const heatmap_palette_t *palette, uint8_t heat, uint8_t val) {
    const rgb_t   *stops    = palette->stops;
    const uint16_t segments = palette->count - 1;
    const uint16_t span     = 256;
    uint16_t       pos      = heat * segments; // 0 .. 255 * segments
    uint8_t        i        = pos / span;
    uint16_t       t        = pos % span;
    rgb_t          a        = stops[i];
    rgb_t          b        = stops[i < segments ? i + 1 : i];
    return (rgb_t){
        lerp8(a.r, b.r, t, span) * val / 255,
        lerp8(a.g, b.g, t, span) * val / 255,
        lerp8(a.b, b.b, t, span) * val / 255,
    };
}

// QMK's heatmap tracks how hot each key is in g_rgb_frame_buffer, but turns cold
// keys and underglow off and goes from blue to red. Repaint it with our own colours after
// every frame. Runs on both halves, since each half renders its own LEDs.
bool rgb_matrix_indicators_advanced_user(uint8_t led_min, uint8_t led_max) {
    if (rgb_matrix_get_mode() != RGB_MATRIX_TYPING_HEATMAP) {
        return false;
    }

    uint8_t                  hue     = rgb_matrix_get_hue();
    const heatmap_palette_t *palette = &heatmap_palettes[hue < ARRAY_SIZE(heatmap_palettes) ? hue : 0];
    uint8_t                  val     = rgb_matrix_get_val();

    // Underglow LEDs aren't under a key, so they just show the cold colour
    rgb_t cold = heatmap_color(palette, 0, val);
    for (uint8_t i = led_min; i < led_max; i++) {
        rgb_matrix_set_color(i, cold.r, cold.g, cold.b);
    }

    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        for (uint8_t col = 0; col < MATRIX_COLS; col++) {
            uint8_t led = g_led_config.matrix_co[row][col];
            if (led < led_min || led >= led_max) {
                continue;
            }
            rgb_t rgb = heatmap_color(palette, g_rgb_frame_buffer[row][col], val);
            rgb_matrix_set_color(led, rgb.r, rgb.g, rgb.b);
        }
    }
    return false;
}

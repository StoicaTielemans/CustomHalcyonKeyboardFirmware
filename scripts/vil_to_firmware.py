#!/usr/bin/env python3
"""Bake a Vial .vil save file into the `stick` keymap as the firmware defaults.

Generates keyboards/splitkb/halcyon/elora/keymaps/stick/keymap.json with the
default layers and encoder mappings.

Macros are not baked in. A macro key that only toggles a layer and shifts the
RGB hue (the old colour-switching trick) becomes a plain TG(n), because
keymap.c now gives every layer its own colour. Any other macro key becomes
KC_NO, with a warning.

Usage:
    python3 scripts/vil_to_firmware.py [path/to/layout.vil]
"""

import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
KEYMAP_DIR = ROOT / "keyboards/splitkb/halcyon/elora/keymaps/stick"
DEFAULT_VIL = ROOT / "layouts/Halcyon_elora_rev2_vial_layout.vil"
LAYOUT = "LAYOUT_elora_hlc"

# Vial still saves some keycode names that QMK has since removed.
LEGACY_KEYCODES = {
    "KC_LCTRL": "KC_LCTL",
    "KC_RCTRL": "KC_RCTL",
    "KC_LSHIFT": "KC_LSFT",
    "KC_RSHIFT": "KC_RSFT",
    "KC_BSPACE": "KC_BSPC",
    "KC_BSLASH": "KC_BSLS",
    "KC_SCOLON": "KC_SCLN",
    "KC_LBRACKET": "KC_LBRC",
    "KC_RBRACKET": "KC_RBRC",
    "KC_PGDOWN": "KC_PGDN",
    "KC_PSCREEN": "KC_PSCR",
    "KC_CAPSLOCK": "KC_CAPS",
    "KC_NUMLOCK": "KC_NUM",
    "KC_SCROLLLOCK": "KC_SCRL",
    "KC_NONUS_BSLASH": "KC_NUBS",
    "KC_NONUS_HASH": "KC_NUHS",
    "KC_APPLICATION": "KC_APP",
    "KC__MUTE": "KC_MUTE",
    "KC__VOLUP": "KC_VOLU",
    "KC__VOLDOWN": "KC_VOLD",
    "KC_GESC": "QK_GESC",
    "RESET": "QK_BOOT",
}


HUE_KEYCODES = {"RGB_HUI", "RGB_HUD", "RM_HUEU", "RM_HUED", "UG_HUEU", "UG_HUED"}


def colour_macro_layer_toggles(vil):
    """{macro index: "TG(n)"} for macros that are just TG(n) plus hue changes."""
    toggles = {}
    for i, macro in enumerate(vil.get("macro", [])):
        keys = [kc for action, *args in macro for kc in args] if all(a[0] == "tap" for a in macro) else []
        if keys and re.fullmatch(r"TG\(\d+\)", keys[0]) and set(keys[1:]) <= HUE_KEYCODES:
            toggles[i] = keys[0]
    return toggles


def keycode(kc, macro_toggles):
    """Translate one Vial keycode (string or -1) into a QMK C expression."""
    if kc == -1:
        return "KC_NO"
    macro = re.fullmatch(r"M(\d+)", kc)
    if macro:
        if int(macro.group(1)) in macro_toggles:
            return macro_toggles[int(macro.group(1))]
        print(f"warning: macro key {kc} is not baked into the firmware, using KC_NO")
        return "KC_NO"
    return re.sub(r"\b[A-Z][A-Z0-9_]*\b", lambda m: LEGACY_KEYCODES.get(m.group(0), m.group(0)), kc)


def layout_matrix_order():
    """(row, col) of every LAYOUT_elora_hlc argument, parsed from config.h."""
    config = (KEYMAP_DIR / "config.h").read_text()
    args = re.search(r"#define\s+" + LAYOUT + r"\(([^)]*)\)", config).group(1)
    return [(int(m.group(1)), ord(m.group(2)) - ord("A")) for m in re.finditer(r"k(\d+)([A-G])", args)]


def write_keymap_json(vil, vil_path):
    order = layout_matrix_order()
    toggles = colour_macro_layer_toggles(vil)
    # Group the flat argument list the same way the physical keyboard is laid out.
    row_sizes = [12, 12, 12, 16, 10, 10]
    assert sum(row_sizes) == len(order)

    layers = []
    for layer in vil["layout"]:
        keys = [keycode(layer[r][c], toggles) for r, c in order]
        rows, i = [], 0
        for size in row_sizes:
            rows.append(", ".join(json.dumps(k) for k in keys[i : i + size]))
            i += size
        layers.append("        [\n            " + ",\n            ".join(rows) + "\n        ]")

    encoders = []
    for layer in vil.get("encoder_layout", []):
        encoders.append(
            "        [" + ", ".join(json.dumps({"ccw": keycode(ccw, toggles), "cw": keycode(cw, toggles)}) for ccw, cw in layer) + "]"
        )

    out = (
        "{\n"
        f'    "notes": "Generated from {vil_path.name} by scripts/vil_to_firmware.py - do not edit by hand",\n'
        '    "keyboard": "splitkb/halcyon/elora/rev2",\n'
        '    "keymap": "stick",\n'
        '    "version": 1,\n'
        f'    "layout": "{LAYOUT}",\n'
        '    "layers": [\n' + ",\n".join(layers) + "\n    ]"
    )
    if encoders:
        out += ',\n    "encoders": [\n' + ",\n".join(encoders) + "\n    ]"
    out += "\n}\n"
    (KEYMAP_DIR / "keymap.json").write_text(out)
    return len(layers)


def main():
    vil_path = Path(sys.argv[1]) if len(sys.argv) > 1 else DEFAULT_VIL
    vil = json.loads(vil_path.read_text())

    for feature in ("tap_dance", "combo", "key_override", "alt_repeat_key"):
        if any(set(re.findall(r'"([A-Z][A-Z0-9_()]*)"', json.dumps(e))) - {"KC_NO"} for e in vil.get(feature, [])):
            print(f"warning: {feature} entries in the .vil are not baked into the firmware")

    n_layers = write_keymap_json(vil, vil_path)
    print(f"Wrote {n_layers} layers and encoders to {(KEYMAP_DIR / 'keymap.json').relative_to(ROOT)}")


if __name__ == "__main__":
    main()

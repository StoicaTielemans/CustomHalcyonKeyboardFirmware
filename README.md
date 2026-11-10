# Custom Vial firmware for my Halcyon Elora rev2

Custom [Vial](https://get.vial.today/) firmware for a **splitkb Halcyon Elora rev2** (RP2040 Halcyon wired controller, no Halcyon module installed).

This repo is a fork of [splitkb/qmk_userspace](https://github.com/splitkb/qmk_userspace) (branch `halcyon`, the Vial branch, as recommended by the [splitkb docs](https://docs.splitkb.com/product-guides/halcyon-series/resources/compiling-firmware-qmk)). The original splitkb README is kept further down.

Everything needed to build (the QMK CLI, the ARM compiler and the Vial source) is installed **inside this repo** in `.qmk/`. Nothing is installed system-wide and no `sudo` is needed.

**What this firmware does** compared to the stock splitkb Vial firmware:

- **My layout is baked in** (from `layouts/Halcyon_elora_rev2_vial_layout.vil`), so a freshly flashed keyboard is ready to use without loading anything in Vial.
- **Two layers:** layer 0 is the QWERTY base, layer 1 has the F-keys, navigation and media keys. Two right-hand thumb keys always switch between them (`TO(1)` / `TO(0)`).
- **Lighting per layer:** layer 0 is a typing heatmap that's red at rest and fades through cream to light blue as you type, and layer 1 is a heatmap that's white at rest and turns sage, then olive. See [Layer colours](#layer-colours).
- **No Vial macros.** The old macros only faked layer colours. The firmware does that now.

---

## Quick reference

```sh
./scripts/setup.sh              # once, after cloning
source scripts/env.sh           # in every new terminal
qmk userspace-compile           # build -> splitkb_halcyon_elora_rev2_stick.uf2

python3 scripts/vil_to_firmware.py   # after saving a new layout from Vial (then build again)
```

---

## Step-by-step

### 1. Requirements

These must already be on your system (all are standard on CachyOS/Arch):

| Tool | Check with | Used for |
| :--- | :--- | :--- |
| `git` | `git --version` | cloning the Vial source |
| `uv` | `uv --version` | creating the Python venv (`sudo pacman -S uv` if missing) |
| `curl`, `tar` with zstd | `tar --zstd --help` | downloading the prebuilt compiler |

You'll need about **3 GB of free disk space** for `.qmk/`.

### 2. Clone this repo

```sh
git clone https://github.com/StoicaTielemans/qmk_userspace.git CustomHalcyonKeyboardFirmware
cd CustomHalcyonKeyboardFirmware
```

### 3. Run the one-time setup

```sh
./scripts/setup.sh
```

The script is safe to run again. It skips steps that are already done. It creates:

| Path | Contents |
| :--- | :--- |
| `.qmk/venv/` | Python virtual environment holding the `qmk` CLI and Vial's Python requirements |
| `.qmk/toolchain/` | Prebuilt ARM GCC from [qmk/qmk_toolchains](https://github.com/qmk/qmk_toolchains) (replaces `pacman -S arm-none-eabi-gcc`) |
| `.qmk/vial-qmk/` | Shallow clone of [vial-kb/vial-qmk](https://github.com/vial-kb/vial-qmk) with submodules |
| `.qmk/bin/qmk` | Wrapper that makes `qmk` use the config in `.qmk/config/` instead of `~/.config/qmk/` |
| `.qmk/config/qmk/qmk.ini` | Points `qmk_home` at `.qmk/vial-qmk` and `overlay_dir` at this repo |

`.qmk/` is in `.gitignore`, so none of this is committed.

### 4. Activate the environment

In **every new terminal** (works in zsh and bash):

```sh
source scripts/env.sh
```

Like activating a Python venv, this only affects the current terminal. It puts `.qmk/bin`, `.qmk/venv/bin` and `.qmk/toolchain/bin` at the front of `PATH`. To check that it worked:

```sh
which qmk          # -> .../.qmk/bin/qmk
qmk env            # QMK_FIRMWARE should be .../.qmk/vial-qmk
```

### 5. Build

```sh
qmk userspace-compile
```

This builds every target in `qmk.json`. The firmware ends up in the repo root:

```
splitkb_halcyon_elora_rev2_stick.uf2
```

(`*.uf2` files are git-ignored.)

To build one target manually, or to see the exact compile command:

```sh
qmk compile -kb splitkb/halcyon/elora/rev2 -km stick -e HLC_NONE=1 -e TARGET=splitkb_halcyon_elora_rev2_stick
qmk userspace-compile -n     # print commands without building
```

### 6. Flash both halves

Each half has its own controller with its own copy of the firmware, so **both halves must be flashed, one at a time**, with the same `.uf2` file. There's no separate left or right firmware; each half detects its side by itself. If only one half is flashed, the halves may disagree (for example on layers or colours).

The Elora uses an RP2040, so flashing is just a file copy. No flashing tool, udev rules or `sudo` are needed.

1. **Unplug everything,** including the USB-C cable that links the two halves.
2. Plug **only the left half** straight into the PC.
3. Enter the bootloader with **any** of these:
   - **double-tap** the reset button (top of the controller, above the USB-C port)
   - hold **Boot** while plugging in
   - press a `QK_BOOT` key, if your layout has one
4. Check that it's in bootloader mode:
   ```sh
   lsusb | grep -iE '2e8a|splitkb'
   ```
   - `2e8a:0003 Raspberry Pi RP2 Boot` means it's in the bootloader and ready.
   - `8d1d:a392 splitkb.com Halcyon Elora rev2` means it's still running normally. Double-tap reset again.

   To watch it happen live, run `sudo dmesg -w` (or `journalctl -kf`) while you press reset.
5. A drive called **`RPI-RP2`** appears. Check that it's mounted, and mount it if it isn't:
   ```sh
   lsblk -o NAME,LABEL,SIZE,MOUNTPOINT | grep -i RPI      # no mount point = not mounted
   udisksctl mount -b /dev/disk/by-label/RPI-RP2          # prints where it mounted
   ```
6. Copy the firmware onto it:
   ```sh
   cp splitkb_halcyon_elora_rev2_stick.uf2 /run/media/$USER/RPI-RP2/ && sync
   ```
   The drive disconnects by itself after a few seconds; an "error ejecting" message is normal. Check that it worked: `lsusb` shows `splitkb.com Halcyon Elora rev2` again.
7. Unplug it and repeat steps 2–6 with **only the right half**.
8. Plug the cable between the halves back in, then connect the keyboard to the PC as usual.

You can always go back to the stock firmware by flashing splitkb's `splitkb_halcyon_elora_rev2_vial_hlc.uf2` the same way. The bootloader can't be overwritten, so flashing can't brick the keyboard.

### 7. Check it in Vial

My layout is **baked into the firmware**, so after flashing the keyboard already has the base layer, the F-keys layer and the encoder mappings. You don't need to load anything.

1. Open Vial ([get.vial.today](https://get.vial.today/), or the desktop app) to check or tweak the keymap.
2. If Vial asks to **unlock** the keyboard, hold the unlock keys it highlights (matrix positions row 0 col 1 + row 6 col 1, i.e. the top-row `5` and `6` keys).

Changes made in Vial are saved on the keyboard and survive unplugging, but **flashing a new build resets the keyboard to the baked-in layout**. To keep a Vial change permanently, bake it in (see [Changing the default layout](#changing-the-default-layout)).

**Quick test:** all keys should be red, and keys you type on should turn purple or blue. Press one of the `TO(1)` thumb keys and both halves should turn white. Press it again (it's `TO(0)` on layer 1) to go back to red.

> Don't load an old `.vil` with colour macros on top of this firmware. Those macros aren't needed anymore (see [Layer colours](#layer-colours)).

---

## Changing the default layout

The default keymap is generated from a Vial save file:

1. Change the layout in Vial.
2. **File → Save current layout** over `layouts/Halcyon_elora_rev2_vial_layout.vil`.
3. Regenerate `keymap.json` from it, then build and flash:
   ```sh
   python3 scripts/vil_to_firmware.py
   source scripts/env.sh && qmk userspace-compile
   ```

What the script bakes in, and what it doesn't:

| From the `.vil` | Baked in? |
| :--- | :--- |
| Layers (all 8) | Yes, into `keymap.json` |
| Encoder mappings | Yes, into `keymap.json` |
| Macros that are `TG(n)` + RGB hue taps (the old colour trick) | Replaced by a plain `TG(n)` key |
| Other macros | No. The key becomes `KC_NO` and the script prints a warning. |
| Tap dance, combos, key overrides | No. The script prints a warning if any are set. |
| QMK settings (tapping term etc.) | No, but mine are all the firmware defaults anyway |

Don't edit `keymap.json` by hand. It's overwritten every time the script runs.

---

## Layer colours

`keymap.c` gives every layer its own lighting. It follows the highest active layer and works on both halves:

| Layer | Contents | Lighting |
| :---: | :--- | :--- |
| 0 | Base (QWERTY) | **Typing heatmap.** Every key and the underglow are brick red at rest. Keys you press shift through cream (papaya whip) to steel blue the more you type, then fade back to red. |
| 1 | F-keys / nav / media | **Typing heatmap.** White smoke at rest, shifting through dry sage to dusty olive as keys get hot. Heat carries over when you switch between layers 0 and 1. |
| 2–7 | empty (free to use in Vial) | Solid green, purple, yellow, cyan, magenta, orange |

Switching layers:

| Key | On layer | Does |
| :--- | :--- | :--- |
| `TO(1)` (the two right-hand thumb keys next to `LAlt`) | 0 | Always go to layer 1 |
| `TO(0)` (the same two keys) | 1 | Always go back to layer 0 |

`TO(n)` turns on layer `n` and turns off every other layer, so pressing it twice doesn't toggle back. Keys marked ▽ on layer 1 (transparent) fall through to the base layer.

Tweaking it in `keymap.c` (rebuild and flash afterwards):

| What | Where |
| :--- | :--- |
| Heatmap colours | `heatmap_layer0` / `heatmap_layer1` in `keymap.c`: lists of RGB colours from cold (rest) to hot, blended evenly. Add or remove stops to change a gradient. |
| Colours of layers 2–7 | the `layer_hues` table |
| Different lighting for a layer | the `switch` in `set_layer_lighting()` |
| How fast keys heat up and cool down | add to `config.h`: `RGB_MATRIX_TYPING_HEATMAP_INCREASE_STEP` (default 32 per press), `RGB_MATRIX_TYPING_HEATMAP_DECREASE_DELAY_MS` (default 25 ms per step), `RGB_MATRIX_TYPING_HEATMAP_SPREAD` (default 40, how far heat spreads to nearby keys) |

Hue runs from 0 to 255: 0 red, 21 orange, 43 yellow, 85 green, 128 cyan, 170 blue, 191 purple, 213 magenta.

- **Brightness** still comes from Vial's Lighting tab.
- **The effect and colour** are set by the layer. Anything picked in Vial is overwritten on the next layer change.

---

## What's in this repo

| Path | What it is |
| :--- | :--- |
| `keyboards/splitkb/halcyon/elora/keymaps/stick/` | **My keymap**, copied from splitkb's `vial_hlc_legacy` |
| `layouts/Halcyon_elora_rev2_vial_layout.vil` | My Vial layout. The source of the default keymap. Based on my original save from `~/Dotfiles`, but with the base layer kept as layer 0, the F-keys layer moved from layer 3 to layer 1, the other layers and all macros removed, and the macro keys replaced by `TO(1)` / `TO(0)`. |
| `qmk.json` | Build targets. Only my keymap is listed. |
| `scripts/setup.sh` | One-time setup of `.qmk/` |
| `scripts/env.sh` | Activates the repo-local environment |
| `scripts/vil_to_firmware.py` | Generates `keymap.json` from the `.vil` |
| `users/halcyon_modules/` | splitkb code for Halcyon modules (display, encoder, trackpad) |

### Files in the `stick` keymap

| File | Purpose |
| :--- | :--- |
| `keymap.json` | Default layers and encoders. **Generated** by `scripts/vil_to_firmware.py`. |
| `keymap.c` | Per-layer RGB lighting: heatmaps on layers 0 and 1, solid colours on layers 2–7 |
| `config.h` | Vial UID, unlock combo, layer count (8), RGB matrix options, legacy matrix layout |
| `rules.mk` | Enables Vial, VialRGB, the encoder map and the Halcyon modules userspace |
| `vial.json` | Tells the Vial app the physical layout and matrix size (12 rows × 7 cols) |
| `halcyon_overrides.c` | LED positions and matrix mask for the legacy matrix layout |

Other custom C code goes in `keymap.c` too.

### Why `vial_hlc_legacy` and not `vial_hlc`?

The stock firmware this keyboard shipped with (`splitkb_halcyon_elora_rev2_vial_hlc.uf2`) uses the **legacy** 12×7 matrix with UID `0x91C072B1EA3FF0BA`. A `.vil` file only loads correctly on firmware with the same matrix, and `scripts/vil_to_firmware.py` relies on it too. The newer `vial_hlc` keymap uses a 10×12 matrix, so all keys would end up in the wrong place.

**Do not change `VIAL_KEYBOARD_UID` or the matrix**, or `.vil` files won't load anymore.

---

## Halcyon modules

`qmk.json` builds with `HLC_NONE=1` (no module installed). If you add a module, change that flag and rebuild:

| Flag | Module |
| :--- | :--- |
| `HLC_NONE` | No module on this half (module on the other half, or none at all) |
| `HLC_ENCODER` | Rotary encoder module |
| `HLC_ENCODER_REV2` | Rotary encoder module, revision 2 |
| `HLC_TFT_DISPLAY` | TFT LCD display module |
| `HLC_CIRQUE_TRACKPAD` | Cirque trackpad module |

The easiest way to add a target is:

```sh
qmk userspace-add -kb splitkb/halcyon/elora/rev2 -km stick -e HLC_ENCODER=1 -e TARGET=splitkb_halcyon_elora_rev2_stick_encoder
```

Each half gets the firmware that matches **its own** module.

---

## Building with GitHub Actions instead

Every push to GitHub also builds the firmware in the cloud (`.github/workflows/build_binaries.yaml`):

1. On GitHub, open the **Actions** tab and click **"I understand my workflows, go ahead and enable them"** (first time only).
2. Push a commit.
3. Download the `.uf2` from the **Releases** tab (release `latest-halcyon-vial`).

---

## Troubleshooting

| Problem | Fix |
| :--- | :--- |
| `qmk: invalid choice: 'userspace-compile'` | The environment isn't active. Run `source scripts/env.sh`. |
| `qmk env` shows `QMK_FIRMWARE=""` | `.qmk/vial-qmk` is missing or incomplete. Run `./scripts/setup.sh` again. |
| `arm-none-eabi-gcc: command not found` | Run `source scripts/env.sh`. If it still fails, delete `.qmk/toolchain` and run setup again. |
| `qmk doctor` warns about udev rules | Safe to ignore. RP2040 boards flash by file copy. |
| `RPI-RP2` is in `lsblk` but has no mount point | `udisksctl mount -b /dev/disk/by-label/RPI-RP2` |
| `RPI-RP2` drive doesn't appear | Use a data USB cable, plug that half straight into the PC (not through the USB-C cable that links the halves), and try the Boot + plug-in method. |
| Halves don't talk to each other | Flash **both** halves with the same firmware. |
| Layer colours don't change | Check that RGB is on and the brightness isn't 0 (Vial → Lighting). Make sure **both** halves have the new firmware. |
| Heatmap colours look wrong after a Vial change | The effect is set by the layer. Switch layers once, or re-flash to reset everything. |
| Keys are in the wrong place in Vial | The firmware has the wrong matrix. Use the `stick` (legacy) keymap, not `vial_hlc`. |
| Start over completely | `rm -rf .qmk && ./scripts/setup.sh` |

### Updating

```sh
git -C .qmk/vial-qmk pull && git -C .qmk/vial-qmk submodule update --init --recursive   # newer Vial
git fetch splitkb && git merge splitkb/halcyon                                            # newer splitkb userspace
```

(Add the splitkb remote once with `git remote add splitkb https://github.com/splitkb/qmk_userspace.git` if it's missing.)

To update the compiler, change `TOOLCHAIN_VERSION`/`TOOLCHAIN_ASSET` in `scripts/setup.sh`, then run `rm -rf .qmk/toolchain && ./scripts/setup.sh`.

---
---

# Original splitkb README: Halcyon Modules QMK Userspace

This is the splitkb userspace repository. It allows for an external set of QMK keymaps with **Halcyon modules** to be defined and compiled without having to fork the main QMK or Vial repositories. 

*If you want to compile standard firmware without any Halcyon modules, you can use the [main qmk_userspace repo](https://github.com/qmk/qmk_userspace). If you use the Halcyon to Promicro adapter board without any Halcyon modules you can use the converter without this repository.*

## Supported Keyboards

Supported controllers:

| Controller name |
| :--- |
| Halcyon Wired controller |


Supported keyboards:

| Keyboard name | Keyboard variable |
| :--- | :--- |
| Halcyon Kyria (rev4) | `splitkb/halcyon/kyria/rev4` |
| Halcyon Elora (rev2) | `splitkb/halcyon/elora/rev2` |
| Halcyon Corne (rev2) | `splitkb/halcyon/corne/rev2` |
| Halcyon Ferris (rev1) | `splitkb/halcyon/ferris/rev1` |
| Halcyon Lily58 (rev2) | `splitkb/halcyon/lily58/rev2` |
| Aurora Sweep (rev1)* | `splitkb/aurora/sweep/rev1` |
| Aurora Lily58 (rev1)* | `splitkb/aurora/lily58/rev1` |
| Aurora Corne (rev1)* | `splitkb/aurora/corne/rev1` |
| Aurora Helix (rev1)* | `splitkb/aurora/helix/rev1` |
| Aurora Sofle v2 (rev1)* | `splitkb/aurora/sofle_v2/rev1` |
| Kyria (rev3)* | `splitkb/kyria/rev3` |

*Requires the Halcyon converter


Supported converters:

| Converter name | Converter variable | Convert command |
| :--- | :--- | :--- |
| Halcyon | `halcyon` | `CONVERT_TO=halcyon` |


Supported modules:

| Module name | Module variable |
| :--- | :--- |
| [Halcyon TFT LCD Display Module](https://splitkb.com/products/halcyon-tft-lcd-display-module) | `HLC_TFT_DISPLAY` |
| [Halcyon Rotary Encoder Module](https://splitkb.com/products/halcyon-rotary-encoder-module) | `HLC_ENCODER` |
| [Halcyon Rotary Encoder Module Revision 2](https://splitkb.com/products/halcyon-rotary-encoder-module) | `HLC_ENCODER_REV2` |
| [Halcyon Cirque Touchpad Module](https://splitkb.com/products/halcyon-cirque-touchpad-module) | `HLC_CIRQUE_TRACKPAD` |

If you want to add a keyboard which doesn't have support for Halcyon modules yet, please follow the [porting guide](docs/PORTING.md). Please follow the [initial setup](#initial-setup--prerequisites) and [build target](#how-to-configure-your-build-targets) steps from this readme first.

This repository contains keymaps for both upstream QMK (default_hlc) and Vial (vial_hlc). For the officially supported keyboards we will always provide these two keymaps. 


## Initial Setup & Prerequisites

Before configuring your keymaps or building firmware, you need to set up your build environment. 

1. **Fork this repository** to your own GitHub account. 
   - The default `halcyon` branch includes Vial support.
   - If you prefer upstream QMK without Vial, fork the `halcyon-qmk` branch instead.
2. **Clone the Vial repository** to your local machine:
   - For Vial:
     ```bash
     git clone --recursive https://github.com/vial-kb/vial-qmk
     ```
   - For upstream QMK:
     ```bash
     git clone --recursive https://github.com/qmk/qmk_firmware
     ```
3. **Set up QMK** using your cloned repository (see [QMK Docs](https://docs.qmk.fm/#/newbs) for more details):
   ```bash
   qmk setup -H path/to/your/qmk/repository
   ```
   When prompted whether to keep the existing repository, select **3**.
4. **Clone your forked userspace repository** to your local machine.
   ```bash
   git clone https://github.com/YOUR_GITHUB_USERNAME/qmk_userspace
   ```
5. **Enable userspace in QMK config**: 
   Navigate into your cloned userspace directory and run:
   ```bash
   cd path/to/your/forked/qmk_userspace
   qmk config user.overlay_dir="$(realpath .)"
   ```
   This tells QMK where to find your userspace, regardless of your current working directory.

*Note: If you have already forked the `qmk/qmk_userspace` repository previously, see the [Adding to an Existing Fork](#adding-halcyon-support-to-an-existing-userspace-fork) section below.*


## How to Configure Your Build Targets

1. **Start fresh (Optional but recommended):** If you want to start completely from scratch without any default compile options, replace the `qmk.json` in the root folder with the provided `qmk_empty.json`.
2. **Create your keymap:** Navigate to `keyboards/<keyboard_name>/keymaps` and copy/paste the `default_hlc` or the `vial_hlc` or `vial_hlc_legacy` folder, if you still want to use vial. Rename it to your desired keymap name.  
    *(**Updating Keymaps:** If you modify an existing keymap (e.g., from the original Kyria, Elora, or Aurora), and want to use a Halcyon encoder module, make sure to **add the Halcyon Button mappings** and **Encoder mapping** as shown in [the porting guide](docs/PORTING.md#3-define-halcyon-button-mappings))*  
    *(If you're unsure what the exact `keyboard_name` is, you can run `qmk list-keyboards | grep <keyboard>`)*  
    *For existing Vial users, you should use the `vial_hlc_legacy` keymap as the location mapping is different in the newer version and this is not compatible with the new version. You would otherwise have to recreate your keymap in Vial. Otherwise you can use the newer `vial_hlc` keymap.*
3. **Add your keymap to the build targets** by running the following command:
   ```bash
   qmk userspace-add -kb <your_keyboard> -km <your_keymap> -e <halcyon_module>=1 -e TARGET=<filename>
   ```
   * *This command will automatically update your `qmk.json` file.*
   * **`filename`**: Choose a descriptive filename so you can easily differentiate between module firmware (e.g., `halcyon_kyria_default_encoder`).
   * **`halcyon_module`**: Replace this with one of the following environment variables depending on your hardware:

   | Module Variable | Description |
   | :--- | :--- |
   | `HLC_NONE` | You have a module installed on the *other* half, but not this half. |
   | `HLC_ENCODER` | You have an encoder module installed. |
   | `HLC_ENCODER_REV2` | You have a second revision encoder module installed. |
   | `HLC_TFT_DISPLAY` | You have a TFT RGB display installed. |
   | `HLC_CIRQUE_TRACKPAD` | You have a Cirque trackpad installed. |

#### Useful Userspace Commands
* **List configured targets:** `qmk userspace-list`
* **Show generated compile commands:** `qmk userspace-compile -n`
* **Remove a target:** `qmk userspace-remove -kb <your_keyboard> -km <your_keymap> -e <halcyon_module>=1 -e TARGET=<filename>`. You can also remove a target by removing the specific lines in the `qmk.json` file.


## Changing module behavior

Please see the [seperate docs](docs/MODULES.md).


## How to Build with GitHub Actions

If you don't want to build locally, GitHub can compile the firmware for you automatically.

1. Go to the **Actions** tab of your forked GitHub repository and click **"I understand my workflows, go ahead and enable them"**.
2. Commit and push your local changes to your fork.
3. Check the **Actions** tab to watch the build process run.
4. Once completed, navigate to the **Releases** tab on your repository to download your latest compiled firmware `.hex` or `.uf2` files.


## How to Build Locally

Assuming you have completed the initial setup and configured your build targets, you can compile locally.

**To build all userspace targets at once:**
```bash
qmk userspace-compile
```

**To compile a specific target manually:**
```bash
qmk compile -kb <your_keyboard> -km <your_keymap> -e <your_module>=1 -e TARGET=<filename>
```
*Tip: use `qmk userspace-compile -n` to get the exact compile command*

## Extra Info & Advanced Configuration

### Modifying GitHub Actions
If you wish to point GitHub Actions to a different QMK repository (such as the main QMK repo instead of Vial), or a different branch, you can modify `.github/workflows/build_binaries.yml`.

For example, to override the `build` job to use the QMK branch (which is already done in the `halcyon-qmk` branch):
```yaml
    with:
      qmk_repo: qmk/qmk_firmware
      qmk_ref: master
```
*Note: Our Halcyon module code should work fine with the main QMK repository, but it may break if QMK introduces upstream breaking changes. We track QMK updates in the `halcyon-qmk` branch.*

### Adding Halcyon support to an Existing Userspace Fork

If you already have a custom userspace fork of `qmk/qmk_userspace`, you can merge the Splitkb additions manually.

#### Option A: Adding the Splitkb Upstream Branch

1. Add this repository as a remote:
   ```bash
   git remote add splitkb https://github.com/splitkb/qmk_userspace.git
   ```
2. Fetch the upstream branches:
   ```bash
   git fetch splitkb
   ```
3. Create a local branch based on the appropriate upstream branch:
   - **Vial:**
     ```bash
     git checkout -b halcyon splitkb/halcyon
     ```
   - **Upstream QMK:**
     ```bash
     git checkout -b halcyon-qmk splitkb/halcyon-qmk
     ```
4. Make your changes and push your branch:
   ```bash
   git push -u origin <branch-name>
   ```
   Replace `<branch-name>` with either `halcyon` or `halcyon-qmk`.

#### Option B: Copying files to your existing branch
1. Clone or download the files from the Splitkb fork.
2. Copy and/or merge `users/halcyon_modules/rules.mk` and the entire `users/halcyon_modules/splitkb/` folder into your personal user folder.

**⚠️ Warning:** We use some quantum functions in our userspace. If your existing userspace relies heavily on custom quantum functions, you may encounter compile conflicts. If you restrict yourself to `_user` functions, you should be fine.

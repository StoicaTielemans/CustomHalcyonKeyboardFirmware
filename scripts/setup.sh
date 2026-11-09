#!/usr/bin/env bash
# One-time setup of a self-contained QMK/Vial build environment in .qmk/
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
QMK_LOCAL="$ROOT/.qmk"
TOOLCHAIN_VERSION="v15.2.0-1"
TOOLCHAIN_ASSET="qmk_toolchains-gcc15.2.0-host_linuxX64.tar.zst"

mkdir -p "$QMK_LOCAL"

echo "==> Python venv with the QMK CLI"
uv venv --quiet --allow-existing "$QMK_LOCAL/venv"
VIRTUAL_ENV="$QMK_LOCAL/venv" uv pip install --quiet qmk

echo "==> ARM compiler (prebuilt QMK toolchain)"
if [ ! -x "$QMK_LOCAL/toolchain/bin/arm-none-eabi-gcc" ]; then
    rm -rf "$QMK_LOCAL/toolchain" && mkdir -p "$QMK_LOCAL/toolchain"
    curl -fL --progress-bar \
        "https://github.com/qmk/qmk_toolchains/releases/download/$TOOLCHAIN_VERSION/$TOOLCHAIN_ASSET" \
        | tar --zstd -x -C "$QMK_LOCAL/toolchain" --strip-components=1
fi

echo "==> Vial firmware source"
if [ ! -d "$QMK_LOCAL/vial-qmk/.git" ]; then
    git clone --depth 1 --recurse-submodules --shallow-submodules \
        https://github.com/vial-kb/vial-qmk "$QMK_LOCAL/vial-qmk"
fi
VIRTUAL_ENV="$QMK_LOCAL/venv" uv pip install --quiet -r "$QMK_LOCAL/vial-qmk/requirements.txt"

echo "==> Repo-local qmk wrapper and config (ignores ~/.config/qmk)"
mkdir -p "$QMK_LOCAL/bin" "$QMK_LOCAL/config"
cat > "$QMK_LOCAL/bin/qmk" <<'WRAPPER'
#!/usr/bin/env bash
QMK_LOCAL="$(cd "$(dirname "$0")/.." && pwd)"
XDG_CONFIG_HOME="$QMK_LOCAL/config" exec "$QMK_LOCAL/venv/bin/qmk" "$@"
WRAPPER
chmod +x "$QMK_LOCAL/bin/qmk"
"$QMK_LOCAL/bin/qmk" config user.qmk_home="$QMK_LOCAL/vial-qmk" user.overlay_dir="$ROOT" >/dev/null

echo
echo "Done. Now run:  source scripts/env.sh && qmk userspace-compile"

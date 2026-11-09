# Source this file to use the repo-local QMK toolchain:  source scripts/env.sh
# Everything lives in .qmk/ (git-ignored); nothing is installed system-wide.

QMK_LOCAL="$(cd "$(dirname "${BASH_SOURCE[0]:-${(%):-%x}}")/.." && pwd)/.qmk"

export QMK_HOME="$QMK_LOCAL/vial-qmk"
export QMK_USERSPACE="$(dirname "$QMK_LOCAL")"
export PATH="$QMK_LOCAL/bin:$QMK_LOCAL/venv/bin:$QMK_LOCAL/toolchain/bin:$PATH"
export VIRTUAL_ENV="$QMK_LOCAL/venv"

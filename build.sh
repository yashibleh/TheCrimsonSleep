#!/usr/bin/env bash
# Build THE CRIMSON SLEEP core (C++17) into assets/game.wasm.
# Requires a clang with the wasm32 target + wasm-ld. The reference toolchain is
# Zig 0.13 (ships clang 18 + lld); set ZIG=/path/to/zig if it lives elsewhere.
set -euo pipefail
cd "$(dirname "$0")"

ZIG="${ZIG:-$HOME/toolchain/zig-linux-x86_64-0.13.0/zig}"
if [ ! -x "$ZIG" ]; then
  echo "error: C++/wasm toolchain not found at $ZIG" >&2
  echo "install zig 0.13 and run: ZIG=/path/to/zig ./build.sh" >&2
  exit 1
fi

mkdir -p assets build

# Every extern "C" function in api.cpp is exported to JavaScript.
EXPORT_NAMES=$(grep -E '^extern "C" ' cpp/src/api.cpp | grep -oE 'tcs_[A-Za-z0-9_]+' | sort -u)
EXPORT_FLAGS=""
for name in $EXPORT_NAMES; do
  EXPORT_FLAGS="$EXPORT_FLAGS -Wl,--export=$name"
done

SRC=$(find cpp/src -maxdepth 1 -name '*.cpp' | sort)

# shellcheck disable=SC2086
"$ZIG" c++ \
  -target wasm32-freestanding \
  -std=c++17 -O2 \
  -fno-exceptions -fno-rtti -fno-builtin -nostdlib \
  -Wall -Wextra -Wno-unused-parameter \
  -Icpp/include \
  -Wl,--no-entry \
  -Wl,--export=__heap_base \
  $EXPORT_FLAGS \
  -o assets/game.wasm \
  $SRC

echo "built assets/game.wasm ($(wc -c < assets/game.wasm) bytes, $(( $(echo "$EXPORT_NAMES" | wc -l) )) exports)"

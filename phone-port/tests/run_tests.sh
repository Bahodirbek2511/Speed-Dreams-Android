#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$(mktemp -d)"
trap 'rm -rf "$OUT"' EXIT
CXX="${CXX:-c++}"
"$CXX" -std=c++17 -Wall -Wextra -Werror -pedantic -pthread \
  -I "$ROOT/phone-port/src" \
  "$ROOT/phone-port/src/phone_controls.cpp" \
  "$ROOT/phone-port/tests/phone_controls_test.cpp" \
  -o "$OUT/phone_controls_test"
"$OUT/phone_controls_test"

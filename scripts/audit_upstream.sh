#!/usr/bin/env bash
set -euo pipefail

SRC_DIR="${1:-upstream}"
OUT_FILE="${2:-reports/android-portability-audit.md}"
mkdir -p "$(dirname "$OUT_FILE")"
if [[ ! -f "$SRC_DIR/CMakeLists.txt" ]]; then echo "Missing upstream CMakeLists.txt" >&2; exit 2; fi
REV="$(git -C "$SRC_DIR" rev-parse HEAD 2>/dev/null || echo unknown)"
DATE_UTC="$(date -u +%F)"
TMP="$(mktemp)"
trap 'rm -f "$TMP"' EXIT
grep -nE "CMAKE_MINIMUM_REQUIRED|PROJECT|ADD_SUBDIRECTORY" "$SRC_DIR/CMakeLists.txt" > "$TMP" || true
{
  echo "# Speed Dreams Android portability audit"
  echo
  echo "- Date (UTC): $DATE_UTC"
  echo "- Upstream revision: $REV"
  echo "- Upstream: https://github.com/argos-research/speed-dreams"
  echo
  echo "This source-level audit does not compile Android code and does not produce an APK."
  echo
  echo "## Top-level CMake markers"
  echo
  echo "~~~text"
  cat "$TMP"
  echo "~~~"
  echo
  echo "## Dependency references in source/build files"
  echo
  for pattern in "SDL|SDL.h" "PLIB|plib|ssggraph" "OpenSceneGraph|osggraph" "OpenGL|GLX" "OpenAL" "ENet|enet" "FreeType|freetype" "ANDROID|android_app"; do
    count="$(grep -RIE --include=CMakeLists.txt --include="*.cmake" --include="*.h" --include="*.hpp" --include="*.c" --include="*.cpp" --exclude-dir=.git -l "$pattern" "$SRC_DIR" 2>/dev/null | wc -l)"
    printf -- "- %s: %s matching files\n" "$pattern" "$count"
  done
  echo
  echo "## Android-specific files"
  echo
  find "$SRC_DIR" -type f | grep -Ei "android|/gradlew$|/Android[.]mk$|/Application[.]mk$" | head -50 > "$TMP" || true
  if [[ -s "$TMP" ]]; then echo "~~~text"; cat "$TMP"; echo "~~~"; else echo "No obvious Android NDK or Gradle files found by filename."; fi
  echo
  echo "## Implications"
  echo
  echo "1. The upstream source uses CMake and was designed around desktop dependencies and APIs."
  echo "2. Android needs an NDK-compatible dependency stack, graphics path, activity lifecycle, touch input, audio/storage integration, and packaging."
  echo "3. A Linux build does not prove Android compatibility."
  echo "4. Redmi A1+ performance must be tested on the actual 2 GB RAM device."
  echo
  echo "## Required next gates"
  echo "- [ ] Pin upstream revision and audit source/assets licenses separately."
  echo "- [ ] Confirm all core dependencies build for arm64-v8a."
  echo "- [ ] Port the actual engine and renderer; do not substitute a demo shell."
  echo "- [ ] Package legal game data and integrate Android controls/lifecycle."
  echo "- [ ] Test install, startup, track loading, and a race on the Redmi A1+."
} > "$OUT_FILE"
cat "$OUT_FILE"

#!/usr/bin/env bash
set -euo pipefail

SRC_DIR="${1:-upstream}"
OUT_FILE="${2:-reports/android-portability-audit.md}"
mkdir -p "$(dirname "$OUT_FILE")"

if [[ ! -f "$SRC_DIR/CMakeLists.txt" ]]; then
  echo "ERROR: $SRC_DIR/CMakeLists.txt not found" >&2
  exit 2
fi

REV="$(git -C "$SRC_DIR" rev-parse HEAD 2>/dev/null || echo unknown)"
DATE_UTC="$(date -u +%F)"
TMP="$(mktemp)"
trap 'rm -f "$TMP"' EXIT

{
  echo "# Speed Dreams Android portability audit"
  echo
  echo "- Date (UTC): $DATE_UTC"
  echo "- Upstream: https://github.com/argos-research/speed-dreams"
  echo "- Revision: $REV"
  echo
  echo "> This is a source-level reconnaissance report, not proof of an Android build. No APK is produced by this audit."
  echo
  echo "## Build-system signals"
  echo
  if grep -nE 'CMAKE_MINIMUM_REQUIRED|PROJECT\\(|ADD_SUBDIRECTORY\\(src\\)|ADD_SUBDIRECTORY\\(data\\)' "$SRC_DIR/CMakeLists.txt" > "$TMP"; then
    echo '```text'
    cat "$TMP"
    echo '```'
  else
    echo "No expected top-level CMake markers found."
  fi
  echo
  echo "## Desktop dependency and API references"
  echo
  for pattern in 'SDL 1\\.2|SDL2|SDL.h' 'PLIB|plib|ssggraph' 'OpenSceneGraph|osggraph' 'OpenGL|GL/gl\\.h|GLX' 'OpenAL|AL/al\\.h' 'ENet|enet' 'FreeType|freetype' 'CMAKE_SYSTEM_NAME|ANDROID|android_app'; do
    count="$(grep -RIE --include='CMakeLists.txt' --include='*.cmake' --include='*.h' --include='*.hpp' --include='*.c' --include='*.cpp' --exclude-dir=.git -c "$pattern" "$SRC_DIR" 2>/dev/null | awk -F: '{s+=$NF} END {print s+0}')"
    printf -- '- Pattern %s: %s matching source/build files\n' "$pattern" "$count"
  done
  echo
  echo "## Android-specific build markers"
  echo
  android_files="$(find "$SRC_DIR" -type f \\( -iname '*android*' -o -name 'Android.mk' -o -name 'Application.mk' -o -name 'gradlew' \\) -not -path '*/.git/*' | head -50 || true)"
  if [[ -n "$android_files" ]]; then
    echo '```text'
    printf '%s\n' "$android_files"
    echo '```'
  else
    echo "No obvious Android NDK, Gradle wrapper, or Android.mk/Application.mk files were found by filename scan."
  fi
  echo
  echo "## Initial engineering implications"
  echo
  echo "1. The upstream project uses CMake, but its documented dependency set includes legacy desktop graphics and input/audio libraries."
  echo "2. A native Android build needs an explicit Android toolchain, compatible dependency builds, application lifecycle integration, touch controls, storage handling, and packaging."
  echo "3. A successful Linux configure/build would not establish Android compatibility."
  echo "4. The Redmi A1+ has limited memory and a PowerVR GPU; runtime performance and graphics API support must be measured on-device."
  echo
  echo "## Next required gates"
  echo
  echo "- [ ] Complete dependency/license audit and pin a source revision."
  echo "- [ ] Select a viable Android graphics path and confirm dependencies can be built for arm64-v8a."
  echo "- [ ] Build the real game engine with Android NDK; do not substitute a demo shell."
  echo "- [ ] Package legally distributable game data and integrate Android input/lifecycle."
  echo "- [ ] Test install, startup, track loading, and a race on real hardware."
} > "$OUT_FILE"

cat "$OUT_FILE"

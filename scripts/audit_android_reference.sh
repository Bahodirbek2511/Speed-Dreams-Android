#!/usr/bin/env bash
set -euo pipefail

SRC_DIR="${1:-android-reference}"
OUT_FILE="${2:-reports/android-reference-audit.md}"
mkdir -p "$(dirname "$OUT_FILE")"

for path in CMakeLists.txt android/app/build.gradle android/app/src/main/AndroidManifest.xml src/vr/sd_vr.cpp src/vr/vr_input.cpp; do
  if [[ ! -f "$SRC_DIR/$path" ]]; then
    echo "Missing expected Android reference file: $SRC_DIR/$path" >&2
    exit 2
  fi
done

REV="$(git -C "$SRC_DIR" rev-parse HEAD 2>/dev/null || echo unknown)"
TAG="$(git -C "$SRC_DIR" describe --tags --exact-match 2>/dev/null || echo untagged)"
DATE_UTC="$(date -u +%F)"
MANIFEST="$SRC_DIR/android/app/src/main/AndroidManifest.xml"
GRADLE="$SRC_DIR/android/app/build.gradle"
CMAKE="$SRC_DIR/CMakeLists.txt"

{
  echo "# Android reference audit: SpeedDreams-VR"
  echo
  echo "- Date (UTC): $DATE_UTC"
  echo "- Reference: https://github.com/SgtBilko76/SpeedDreams-VR"
  echo "- Revision: $REV"
  echo "- Tag: $TAG"
  echo
  echo "This is a source inspection, not a phone build. The reference is designed for standalone VR headsets."
  echo
  echo "## Android build configuration"
  echo
  echo '~~~text'
  grep -nE 'compileSdk|ndkVersion|minSdk|targetSdk|versionName|abiFilters|OpenXR|externalNativeBuild' "$GRADLE" || true
  echo '~~~'
  echo
  echo "## VR-only manifest capabilities"
  echo
  echo '~~~text'
  grep -nE 'vr.headtracking|vr_only|supportedDevices|IMMERSIVE_HMD|category.VR|OpenXR|pvr.app.type' "$MANIFEST" || true
  echo '~~~'
  echo
  if grep -qE 'android.hardware.vr.headtracking.*required="true"' "$MANIFEST"; then
    echo "- **Phone compatibility gate: BLOCKED.** The manifest requires android.hardware.vr.headtracking."
  else
    echo "- Head-tracking requirement not detected by the current rule; this does not by itself prove phone compatibility."
  fi
  echo
  echo "## Native OpenXR dependency"
  echo
  echo '~~~text'
  grep -nE 'find_package\(OpenXR|OpenXR::openxr_loader|TBXR_|xrCreateInstance|xrWaitFrame|xrBeginFrame|xrEndFrame' "$CMAKE" "$SRC_DIR/src/vr/sd_vr.cpp" 2>/dev/null | head -70 || true
  echo '~~~'
  echo
  echo "## Controller-based input"
  echo
  echo '~~~text'
  grep -nE 'leftTrackedRemoteState_new|rightTrackedRemoteState_new|IndexTrigger|Joystick|VrJoyRead|VrQueueMouse' "$SRC_DIR/src/vr/vr_input.cpp" | head -60 || true
  echo '~~~'
  echo
  echo "## Conclusion for Redmi A1+"
  echo
  echo "The reference is useful for its Android ARM64 dependency and engine work, but it is not a phone-ready build. A genuine phone port needs a non-OpenXR surface/frame loop, a mono GLES rendering path, touch-driven menu events, and virtual driving controls. Removing the VR feature declaration alone is not sufficient."
  echo
  echo "## Acceptance gates still open"
  echo
  echo "- [ ] Reproduce the reference dependency checkout in clean CI."
  echo "- [ ] Implement a non-OpenXR phone renderer and lifecycle."
  echo "- [ ] Add touch controls for menus and driving."
  echo "- [ ] Build and inspect a real phone APK."
  echo "- [ ] Test installation, startup, track load and a completed race on Redmi A1+."
} > "$OUT_FILE"

cat "$OUT_FILE"

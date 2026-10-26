#!/usr/bin/env bash
set -euo pipefail

ENGINE_URL="${SPEED_DREAMS_CODE_URL:-https://forge.a-lec.org/speed-dreams/speed-dreams-code.git}"
ENGINE_REF="${SPEED_DREAMS_CODE_REF:-main}"
REPORT="${1:-reports/engine-main-compatibility.md}"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
mkdir -p "$(dirname "$REPORT")"

git clone --depth 1 --single-branch --branch "$ENGINE_REF" "$ENGINE_URL" "$WORK/engine"
ENGINE_SHA="$(git -C "$WORK/engine" rev-parse HEAD)"
RACE="$WORK/engine/src/modules/userinterface/legacymenu/racescreens/racerunningmenus.cpp"

{
  echo "# Speed Dreams engine compatibility probe"
  echo
  echo "- Remote: $ENGINE_URL"
  echo "- Branch requested: $ENGINE_REF"
  echo "- Checked commit: $ENGINE_SHA"
  echo "- Checked date (UTC): $(date -u +%F)"
  echo
  if [[ ! -f "$RACE" ]]; then
    echo "- racerunningmenus.cpp: **MISSING** at expected path"
    echo
  else
    echo "- racerunningmenus.cpp: **FOUND**"
    echo
    echo "## Engine hook symbols"
    echo
    echo "| Symbol | Present in source |"
    echo "|---|---|"
    for symbol in VrFrameBegin VrPresent VrRaceFrameBegin VrRaceEyeBegin VrRaceEyeEnd VrRaceFrameEnd; do
      if grep -R -q --include='*.cpp' --include='*.h' --include='*.c' "$symbol" "$WORK/engine/src"; then
        echo "| \`$symbol\` | yes |"
      else
        echo "| \`$symbol\` | no |"
      fi
    done
    echo
    echo "## Race render loop markers"
    echo
    echo '~~~text'
    grep -nE 'VrRace|GfuiRedraw|rmRedisplay|SDL_GL_SwapWindow|glXSwapBuffers|glutSwapBuffers' "$RACE" || true
    echo '~~~'
    echo
    echo "## Required patcher input"
    echo
    if grep -q 'VrRaceEyeBegin' "$RACE"; then
      echo "- Single-eye patcher: **candidate-compatible**"
    else
      echo "- Single-eye patcher: **INCOMPATIBLE**; the expected VR eye hooks are absent from this public engine branch."
      echo "- The phone backend must add frame/present hooks to the upstream renderer before this branch can be built with the custom EGL surface."
    fi
  fi
} > "$REPORT"

cat "$REPORT"

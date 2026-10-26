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

python3 - "$WORK/engine" >> "$REPORT" <<'PY'
import re
import sys
from pathlib import Path

root = Path(sys.argv[1])
patterns = re.compile(r"(?:GfuiRedraw\s*\(|SDL_GL_SwapWindow\s*\(|SDL_GL_SwapBuffers\s*\(|glutSwapBuffers\s*\(|SDL_CreateWindow\s*\(|SDL_GL_CreateContext\s*\(|eventLoop\(\)\.setRedisplayCB)")
print("## Candidate frame-loop implementation sites")
print()
printed = 0
for path in sorted((root / "src").rglob("*")):
    if path.suffix not in {".cpp", ".c", ".h", ".cc"}:
        continue
    try:
        lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
    except OSError:
        continue
    matches = [i for i, line in enumerate(lines) if patterns.search(line)]
    if not matches:
        continue
    for i in matches[:3]:
        start, end = max(0, i - 6), min(len(lines), i + 10)
        print(f"### {path.relative_to(root)}:{i + 1}")
        print("~~~cpp")
        for n in range(start, end):
            print(f"{n + 1}: {lines[n]}")
        print("~~~")
        print()
        printed += 1
        if printed >= 12:
            break
    if printed >= 12:
        break
if printed == 0:
    print("No common SDL/GUI frame-loop implementation markers found.")
PY

python3 - "$WORK/engine" >> "$REPORT" <<'PY'
import re
import sys
from pathlib import Path

root = Path(sys.argv[1])
targets = {
    "src/libs/tgfclient/gui.cpp": re.compile(r"\bGfuiDisplay\s*\(|\bGfuiSwapBuffers\s*\("),
    "src/libs/tgfclient/guiscreen.cpp": re.compile(r"\bSDL_CreateWindow\s*\(|\bSDL_GL_CreateContext\s*\(|\bGfScrInit\b|\bGfScrShutdown\b"),
}
print("## GUI window and swap lifecycle")
print()
for relative, pattern in targets.items():
    path = root / relative
    if not path.exists():
        continue
    lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
    seen = set()
    count = 0
    for index, line in enumerate(lines):
        if not pattern.search(line):
            continue
        # Avoid duplicate context windows around adjacent SDL calls.
        if any(abs(index - prior) <= 8 for prior in seen):
            continue
        seen.add(index)
        start, end = max(0, index - 8), min(len(lines), index + 24)
        print(f"### {relative}:{index + 1}")
        print("~~~cpp")
        for n in range(start, end):
            print(f"{n + 1}: {lines[n]}")
        print("~~~")
        print()
        count += 1
        if count >= 8:
            break
PY


cat "$REPORT"


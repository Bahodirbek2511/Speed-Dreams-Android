#!/usr/bin/env bash
set -euo pipefail

CODE_URL="${SPEED_DREAMS_CODE_URL:-https://forge.a-lec.org/speed-dreams/speed-dreams-code.git}"
DATA_URL="${SPEED_DREAMS_DATA_URL:-https://forge.a-lec.org/speed-dreams/speed-dreams-data.git}"
CODE_REF="${SPEED_DREAMS_CODE_REF:-quest-port}"
REPORT="${1:-reports/android-engine-source-probe.md}"

mkdir -p "$(dirname "$REPORT")"
tmp="$(mktemp)"
trap 'rm -f "$tmp"' EXIT

{
  echo "# Android engine source probe"
  echo
  echo "- Date (UTC): $(date -u +%F)"
  echo "- Engine URL: $CODE_URL"
  echo "- Required ref: $CODE_REF"
  echo "- Data URL: $DATA_URL"
  echo
} > "$REPORT"

echo "Probing engine ref: $CODE_URL ($CODE_REF)"
if git ls-remote --exit-code --heads "$CODE_URL" "refs/heads/$CODE_REF" >"$tmp" 2>&1; then
  engine_sha="$(awk 'NR==1 {print $1}' "$tmp")"
  {
    echo "- Engine branch: **FOUND**"
    echo "- Engine commit observed: $engine_sha"
    echo
  } >> "$REPORT"
  echo "FOUND engine ref $CODE_REF at $engine_sha"
else
  {
    echo "- Engine branch: **NOT AVAILABLE** from this runner"
    echo
    echo '~~~text'
    cat "$tmp"
    echo '~~~'
    echo
    echo "The Android reference CMake expects the quest-port source tree. Do not substitute the older GitHub 2.2 snapshot or label it a reproducible Speed Dreams 2.4 build."
  } >> "$REPORT"
  cat "$REPORT"
  exit 2
fi

: > "$tmp"
echo "Probing game-data repository: $DATA_URL"
if git ls-remote --heads "$DATA_URL" >"$tmp" 2>&1; then
  {
    echo "## Data repository refs"
    echo
    echo '~~~text'
    head -20 "$tmp"
    echo '~~~'
    echo
  } >> "$REPORT"
else
  {
    echo "## Data repository refs"
    echo
    echo "- Data refs: **NOT AVAILABLE** from this runner"
    echo
    echo '~~~text'
    cat "$tmp"
    echo '~~~'
  } >> "$REPORT"
  cat "$REPORT"
  exit 3
fi

cat "$REPORT"

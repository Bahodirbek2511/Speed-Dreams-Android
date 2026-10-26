#!/usr/bin/env bash
set -euo pipefail

CODE_URL="${SPEED_DREAMS_CODE_URL:-https://forge.a-lec.org/speed-dreams/speed-dreams-code.git}"
DATA_URL="${SPEED_DREAMS_DATA_URL:-https://forge.a-lec.org/speed-dreams/speed-dreams-data.git}"
CODE_REF="${SPEED_DREAMS_CODE_REF:-quest-port}"
REPORT="${1:-reports/android-engine-source-probe.md}"

mkdir -p "$(dirname "$REPORT")"
probe="$(mktemp)"
refs="$(mktemp)"
trap 'rm -f "$probe" "$refs"' EXIT

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
if git ls-remote --exit-code --heads "$CODE_URL" "refs/heads/$CODE_REF" >"$probe" 2>&1; then
  engine_sha="$(awk 'NR==1 {print $1}' "$probe")"
  {
    echo "- Required engine branch: **FOUND**"
    echo "- Engine commit observed: $engine_sha"
    echo
  } >> "$REPORT"
  echo "FOUND engine ref $CODE_REF at $engine_sha"
else
  probe_rc=$?
  if git ls-remote --heads "$CODE_URL" >"$refs" 2>&1; then
    {
      if [[ -s "$refs" ]]; then
        echo "- Required engine branch: **MISSING**"
        echo "- The remote responds, but it does not advertise \`$CODE_REF\`."
        echo
        echo "## Branches actually advertised by the engine remote"
        echo
        echo '~~~text'
        sed -E 's/[[:space:]]+refs\/heads\//  /' "$refs" | head -60
        echo '~~~'
        echo
      else
        echo "- Engine remote: **reachable, but no branches were advertised**"
        echo
      fi
    } >> "$REPORT"
  else
    {
      echo "- Engine remote: **UNREACHABLE or Git transport failed** (exit $probe_rc)"
      echo
      echo '~~~text'
      cat "$refs"
      echo '~~~'
      echo
    } >> "$REPORT"
  fi
  echo "Probing game-data remote independently: $DATA_URL"
  if git ls-remote --heads "$DATA_URL" >"$probe" 2>&1; then
    {
      echo "## Data repository refs"
      echo
      echo '~~~text'
      head -30 "$probe"
      echo '~~~'
      echo
    } >> "$REPORT"
  else
    {
      echo "## Data repository refs"
      echo
      echo "- Data remote: **UNREACHABLE or no refs**"
      echo
      echo '~~~text'
      cat "$probe"
      echo '~~~'
      echo
    } >> "$REPORT"
  fi
  cat "$REPORT"
  exit 2
fi

: > "$probe"
echo "Probing game-data repository: $DATA_URL"
if git ls-remote --heads "$DATA_URL" >"$probe" 2>&1; then
  {
    echo "## Data repository refs"
    echo
    echo '~~~text'
    head -30 "$probe"
    echo '~~~'
    echo
  } >> "$REPORT"
else
  {
    echo "## Data repository refs"
    echo
    echo "- Data remote: **UNREACHABLE or no refs**"
    echo
    echo '~~~text'
    cat "$probe"
    echo '~~~'
    echo
  } >> "$REPORT"
  cat "$REPORT"
  exit 3
fi

cat "$REPORT"

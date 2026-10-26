#!/usr/bin/env bash
set -euo pipefail

ROOT="${1:-.}"
cd "$ROOT"

fail() { echo "FAIL: $*" >&2; exit 1; }
pass() { echo "PASS: $*"; }

for f in README.md docs/PORTING_PLAN.md docs/BUILD_STATUS.md docs/ANDROID_PORT_STATUS.md docs/PHONE_PORT_GAP_ANALYSIS.md scripts/audit_upstream.sh scripts/audit_android_reference.sh .github/workflows/upstream-audit.yml; do
  [[ -s "$f" ]] || fail "missing or empty required project file: $f"
done
pass "required project files exist"

bash -n scripts/audit_upstream.sh
bash -n scripts/audit_android_reference.sh
pass "audit shell scripts pass bash syntax checks"

grep -q 'SpeedDreams-VR' .github/workflows/upstream-audit.yml || fail "workflow does not audit Android reference"
grep -q 'audit_android_reference.sh' .github/workflows/upstream-audit.yml || fail "workflow does not run Android phone-gap audit"
grep -qi 'not yet verified' docs/BUILD_STATUS.md || fail "build status must clearly state that device playability is not yet verified"
grep -q 'OpenXR' docs/PHONE_PORT_GAP_ANALYSIS.md || fail "phone gap report must track OpenXR dependency"
grep -q 'touch' docs/PHONE_PORT_GAP_ANALYSIS.md || fail "phone gap report must track touch controls"
pass "workflow and status documentation checks pass"

echo
echo "IMPORTANT: repository-health checks are not a native Android build and do not prove a playable APK exists."

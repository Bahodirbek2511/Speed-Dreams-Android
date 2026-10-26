# Android phone-port status

Last reviewed: 2026-10-26

## Current state

- Universal native engine APK with bundled data: **built in CI**
- Both ARM native ABI sets (`arm64-v8a`, `armeabi-v7a`): **included and checked in CI**
- APK ZIP and v2 signature on the previous candidate: **valid**
- Earlier APK installer attempts on Redmi A1+: **user-reported failure**
- Installer hardening in the current revision: **application ID isolation, stable development signing identity, v1+v2 signing, GLES 2.0 feature minimum, full package checks**
- Actual install/start/race test on Redmi A1+: **not yet performed successfully**

The current work is more than the separate input-only library, but it remains experimental until a physical phone accepts the APK and a full race is completed.

## What the patcher does

The CI build uses the public [SpeedDreams-VR v0.10.0-beta reference](https://github.com/SgtBilko76/SpeedDreams-VR) with a pinned/revision-recorded public Speed Dreams engine checkout. The phone patch replaces OpenXR startup with an Android EGL/GLES surface, adapts the public engine's screen/GUI code, simplifies the render loop to one view, and integrates touch events and driving overlays.

Each build records the resolved source and dependency revisions. Third-party dependencies are checked out from pinned versions. The public engine branch is recorded per run rather than treated as an immutable revision.

## Installer-focused changes

- `applicationId` is `com.speeddreams.mobile`, independent of upstream VR APKs and earlier experimental package identity.
- Both ARM native-library variants are packed into one APK.
- The APK is signed with both v1 and v2; CI verifies the resulting signatures and uses a stable development key to avoid mismatched-signature updates.
- The manifest GLES feature requirement is lowered to GLES 2.0, because the phone EGL code attempts ES3 then ES2.
- CI checks package name, version, min/target SDK, native ABI list, ZIP alignment and bundled data archive integrity.

The stable key is a public development key only. It must never be used for production/Google Play releases; see [signing-key warning](../phone-port/signing/README.md).

## Remaining acceptance gates

1. Install from the direct GitHub Release APK on Redmi A1+.
2. Confirm the first-run storage permission workflow.
3. Verify data extraction, menu interaction, track load, sound and on-screen controls.
4. Finish a race.
5. Measure memory use and sustained performance on the device.

Do not call the port playable until the physical Redmi A1+ tests pass.

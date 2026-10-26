# Build status

Last reviewed: 2026-10-26

## Current result

- Universal ARM APK containing the native engine modules and bundled game data: **CI build PASS** — [previous build #51](https://github.com/Bahodirbek2511/Speed-Dreams-Android/actions/runs/40350874567).
- APK ZIP integrity and v2 signature of that previous candidate: **PASS** — independently checked.
- APK installer compatibility on Redmi A1+: **FAIL on previous candidates**, as reported by the user.
- Installer-hardening revision: separate application ID, stable development certificate, v1+v2 signatures, dual ARM ABIs, and GLES 2.0 manifest minimum; it is rebuilt and validated by the phone APK workflow after merge.
- Touch controls and integration patcher fixtures: **PASS** — [test workflow history](https://github.com/Bahodirbek2511/Speed-Dreams-Android/actions/runs/40300477268).
- Redmi A1+ startup, track/race, audio, controls, memory and FPS: **not yet verified on the physical device**.

The APK is a real build artifact, not the old input-only shared-library artifact. However, package creation and static validation do not by themselves prove installability or a playable game.

## Confirmed technical fixes in the new candidate

- The phone app uses application ID `com.speeddreams.mobile`, separate from the upstream VR reference's `com.speeddreamsvr`.
- Both `arm64-v8a` and `armeabi-v7a` native libraries are included.
- Gradle is instructed to use v1/JAR and v2/APK signing; a stable **development-only** key prevents each sideload build from receiving a different signing identity.
- The manifest no longer advertises the VR reference's GLES 3.1 minimum; the phone EGL code explicitly attempts GLES 3 and falls back to GLES 2.
- CI checks package name/version, min/target SDK, both native ABIs, APK signatures, ZIP alignment, the complete APK ZIP and the bundled game-data ZIP.

## Source and build provenance

The build workflow checks out the public Speed Dreams engine `main` and the `SpeedDreams-VR v0.10.0-beta` Android reference, applies the repository's phone compatibility patcher, checks out pinned native dependencies, records resolved revisions in `android-source-lock.txt`, builds the native app and packages the game data. The public engine revision is recorded per run because the upstream `main` branch itself is not immutable.

The patcher includes compatibility changes for the public engine's GUI/screen initialization and mono race loop. Its fixture tests do not replace a real device test.

## Remaining acceptance gates

1. Install the new signed APK on Redmi A1+ and confirm Package Installer accepts it.
2. Confirm first-run all-files permission handling and extraction of bundled game data.
3. Verify the main menu and track load.
4. Test throttle, brake, steering, camera/pause actions and sound.
5. Complete a race and record frame rate, memory, crashes and heat.

**Playable release APK: not yet verified.** Do not treat a successful CI build as device acceptance.

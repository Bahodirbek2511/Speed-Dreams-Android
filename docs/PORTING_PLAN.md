# Android phone-port plan

## Target device

- Redmi A1+
- 2 GB RAM
- MediaTek Helio A22 / PowerVR GE8320
- Android 12 Go (device configuration may vary)

## Phase 0 — source, provenance and licence

- [ ] Pin an exact source commit and dependency versions.
- [ ] Review the engine, fork modifications, and game-data licences separately.
- [ ] Record all companion repositories, submodules and downloaded libraries.
- [x] Locate an Android/ARM64 engine-port reference: [SpeedDreams-VR](https://github.com/SgtBilko76/SpeedDreams-VR).
- [ ] Establish a clean, reproducible checkout of the reference source and dependencies.

## Phase 1 — phone-rendering architecture

- [x] Confirm the VR reference uses an Android NDK/ARM64 build.
- [x] Confirm that reference requires OpenXR and headset capabilities; it is not phone-compatible unchanged.
- [ ] Replace the OpenXR-owned frame/session loop with a normal Android GLES surface and single-view rendering.
- [ ] Keep the existing engine/menu/physics/audio layers where they can operate without the VR runtime.
- [ ] Define a renderer abstraction so phone mode does not depend on headset-only code.

**Gate:** removing VR metadata is insufficient. Do not call a build a phone port while native startup still requires an OpenXR runtime.

## Phase 2 — Android lifecycle and controls

- [ ] Handle surface creation/destruction, pause/resume, audio focus and rotation.
- [ ] Add a touchscreen menu pointer/tap path.
- [ ] Add an on-screen steering wheel or left/right steering zone plus throttle and brake.
- [ ] Map reverse/gear/camera/menu actions to touch controls.
- [ ] Test on-screen controls at the target phone's aspect ratio and resolution.

## Phase 3 — reproducible native build and packaging

- [ ] Pin Android SDK/NDK, CMake, Gradle and the Android source revision.
- [ ] Recreate all required dependency/source/data checkouts in CI.
- [ ] Build native shared libraries for arm64-v8a.
- [ ] Package only redistributable game data; support external data where required.
- [ ] Inspect the final APK manifest, ABI, minSdk and native library list.
- [ ] Preserve full build logs and APK checksums in GitHub Actions.

## Phase 4 — Redmi A1+ optimization and acceptance

- [ ] Use a low-memory graphics preset and modest internal resolution.
- [ ] Minimize texture and track asset memory; avoid loading the full content catalogue by default.
- [ ] Install and launch on the real Redmi A1+.
- [ ] Verify menus, track loading, race start, touch controls, sound and pause/resume.
- [ ] Complete at least one race and record peak memory, startup time, sustained FPS, crashes and thermal throttling.

## Release criteria

A release is a playable phone APK only when the full game starts without a headset/OpenXR runtime, loads a track, accepts touch controls, and completes a race on the Redmi A1+. A successful CI compile is necessary but not sufficient.

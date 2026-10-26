# Android porting plan

## Target device

- Redmi A1+
- 2 GB RAM
- MediaTek Helio A22 / PowerVR GE8320
- Android 12 Go (device configuration may vary)

## Phase 0 — source and license audit

- [ ] Select a specific upstream release/commit.
- [ ] Review source and asset licenses separately.
- [ ] Identify submodules and external downloads required for a complete build.
- [ ] Record source provenance; do not silently copy a third-party fork as upstream.

## Phase 1 — dependency feasibility

- [ ] Inventory CMake targets and platform-specific source files.
- [ ] Identify every required library and whether it builds for Android NDK/ARM64.
- [ ] Check graphics path: OpenGL ES support versus desktop OpenGL assumptions.
- [ ] Decide whether PLIB/ssggraph can be supported or whether rendering/input needs replacement.
- [ ] Check SDL and audio dependencies for Android-compatible versions.

**Gate:** do not start packaging until the core engine and essential dependencies have a viable Android path.

## Phase 2 — minimal native build

- [ ] Pin Android NDK and CMake versions.
- [ ] Build a minimal native target for arm64-v8a.
- [ ] Add dependency caching only after a successful build.
- [ ] Preserve full compiler logs as GitHub Actions artifacts when builds fail.
- [ ] Do not call a dependency-only or demo build the full game.

## Phase 3 — Android application integration

- [ ] Add Android project/Gradle wrapper.
- [ ] Integrate native libraries through the Android NDK.
- [ ] Handle activity lifecycle, pause/resume, audio focus, and orientation.
- [ ] Add touch controls for steering, throttle, brake, and camera.
- [ ] Package only legally distributable game data.
- [ ] Verify 64-bit ABI and Android minimum SDK.

## Phase 4 — device testing and optimization

- [ ] Install and launch on a real Android device.
- [ ] Verify menus, track loading, race start, input, sound, and pause/resume.
- [ ] Measure peak memory, startup time, and sustained frame rate.
- [ ] Reduce texture size, render resolution, shadows, and effects as needed.
- [ ] Test crashes and thermal throttling on the Redmi A1+.

## Release criteria

A release can be called a playable APK only after the real game engine is packaged and launch/race tests pass. A successful CI compile alone is not proof of playability or compatibility with the target phone.

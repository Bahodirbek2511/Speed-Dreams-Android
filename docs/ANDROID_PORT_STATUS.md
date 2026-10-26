# Android phone-port status

Last reviewed: 2026-10-26

## Current state

- Redmi-compatible game APK: **not built**
- Android ARM64 input/JNI library: **built and symbol-checked in CI**
- Cross-platform touch-control logic: **host-tested**
- Integration patcher: **fixture-tested; not yet validated on the real engine checkout**
- Full native engine build: **blocked by missing engine source branch**
- Install/start/race tested on Redmi A1+: **no**

## Existing Android VR reference

[SpeedDreams-VR](https://github.com/SgtBilko76/SpeedDreams-VR) ports Speed Dreams 2.4 to Android ARM64 for standalone Meta Quest/Pico headsets. Its Android build targets arm64-v8a with NDK 27.2.12479018 and minSdk 29, but its manifest requires VR head-tracking and its native startup requires OpenXR. It cannot be made phone-ready by changing only the manifest.

The VR project expects an adjacent speed-dreams-code checkout on branch quest-port. The [automated source probe](https://github.com/Bahodirbek2511/Speed-Dreams-Android/actions/runs/40299728191) found that the public engine remote no longer advertises this branch. The exact engine modifications and native dependency set required by the VR reference therefore cannot currently be reproduced from the listed public refs alone.

## Phone input work completed

- Thread-safe phone input state for steering, throttle, brake and button bits.
- Multi-touch menu tap/move/release queue with bounded storage.
- JNI declarations and an Android Canvas control overlay.
- Android arm64-v8a native input/JNI shared-library build and export-symbol validation in CI.
- A separate experimental EGL/GLES phone backend and patcher are present under phone-port/, with host tests and isolated source-fixture tests. They have not yet been compiled against the real Speed Dreams engine.

## Remaining implementation and verification

1. Recover or recreate the compatible Android engine source and pin every dependency.
2. Choose one input implementation for the integrated build; do not link both prototype adapters because they export overlapping JNI, joystick, and event symbols.
3. Integrate touch input polling into the engine's event loop and connect VrJoyRead to the full game.
4. Build and verify the non-OpenXR EGL/GLES surface, mono rendering path and race loop against the actual engine sources.
5. Build/sign a full game APK with legally redistributable data, then test menus, track load, driving, audio, pause/resume and a complete race on Redmi A1+.

## Definition of done

The work is complete only when the full game starts without headset/OpenXR services, displays a track, accepts touch controls, completes a race on Redmi A1+, and has measured memory and frame-rate results. The standalone input-library artifact does not satisfy this definition.

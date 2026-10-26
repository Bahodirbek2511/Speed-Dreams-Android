# Phone-port gap analysis

Last reviewed: 2026-10-26

## Executive result

A native Android port of Speed Dreams exists for VR headsets, and this repository now has tested phone-input components. It still does **not** have a genuine Redmi A1+-compatible game APK.

## Evidence from the VR reference

- Source: [SgtBilko76/SpeedDreams-VR](https://github.com/SgtBilko76/SpeedDreams-VR)
- Release: [v0.10.0-beta](https://github.com/SgtBilko76/SpeedDreams-VR/releases/tag/v0.10.0-beta), published 2026-10-20.
- The release APK is approximately 233 MB and targets Meta Quest/Pico class headsets.
- Its Android Gradle config targets arm64-v8a, uses NDK 27.2.12479018, and sets minSdk 29.
- Its manifest requires android.hardware.vr.headtracking, sets VR-only metadata, and declares immersive headset launch categories.
- Its CMake build requires OpenXR::openxr_loader; src/vr/sd_vr.cpp owns the OpenXR session and VR presentation.
- Its controller mapping in src/vr/vr_input.cpp derives steering, throttle, brake and menu actions from headset-controller state.

## Confirmed full-build blocker: missing engine branch

The reference CMake project expects a companion engine tree at ../speed-dreams-code with Android changes from branch quest-port. The CI probe against https://forge.a-lec.org/speed-dreams/speed-dreams-code.git found that the remote is reachable but does not advertise quest-port. It listed other branches, but did not prove those branches include the Android-specific source modifications.

- [Source probe run](https://github.com/Bahodirbek2511/Speed-Dreams-Android/actions/runs/40299728191)
- [Probe report artifact](https://github.com/Bahodirbek2511/Speed-Dreams-Android/actions/runs/40299728191/artifacts/12640256018)

This means the actual game engine cannot be built reproducibly using the current public source refs alone. It is not safe to silently substitute desktop main, because the required Android renderer/lifecycle changes may be missing.

## Work that does pass CI

- [Phone input/JNI ARM64 build](https://github.com/Bahodirbek2511/Speed-Dreams-Android/actions/runs/40300397664): builds the native control library and validates its exported JNI/engine symbols.
- [Host tests and patcher fixtures](https://github.com/Bahodirbek2511/Speed-Dreams-Android/actions/runs/40300477268): touch-control unit tests, Python syntax check and integration patcher test pass.
- These tests validate individual components and source transformations on fixture files only; they do not build or launch the complete game.

## Required implementation

1. Recover the Android engine fork/patches or port them against a pinned engine commit.
2. Reconstruct and pin all native dependencies and game-data inputs, respecting each asset licence.
3. Integrate exactly one phone input implementation; current porting/phone and phone-port are separate prototype layers and must not be linked together because some symbols overlap.
4. Build the non-OpenXR Android EGL/GLES renderer and single-view race path against the real engine.
5. Build and inspect a complete APK, then install it on Redmi A1+ and verify startup, menu touch, track load, driving, sound, pause/resume and a full race.
6. Measure memory, startup and sustained frame rate on the real device.

## Release integrity

Do not rename or re-sign the VR APK as a phone game. Do not call the input-only JNI library an APK or package a blank activity and present it as the game. A release remains blocked until a full native game APK is built and device-tested.

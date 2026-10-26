# Phone-port gap analysis

Last reviewed: 2026-10-26

## Executive result

A native Android port of Speed Dreams exists for VR headsets, but the checked reference is not a flat-screen phone game. This repository does **not** yet have a genuine Redmi A1+-compatible APK.

## Reference and evidence

- Source: [SgtBilko76/SpeedDreams-VR](https://github.com/SgtBilko76/SpeedDreams-VR)
- Latest published release at review time: [v0.10.0-beta](https://github.com/SgtBilko76/SpeedDreams-VR/releases/tag/v0.10.0-beta), published 2026-10-20.
- The release APK is approximately 233 MB and targets Meta Quest/Pico class headsets.
- Its Android Gradle configuration targets arm64-v8a, uses Android NDK 27.2.12479018, and sets minSdk 29.
- Its manifest requires android.hardware.vr.headtracking, sets VR-only metadata, and declares immersive headset launch categories.
- Its CMake build requires OpenXR::openxr_loader; the native entry/frame code in src/vr/sd_vr.cpp owns the OpenXR session and VR presentation.
- Its controller mapping in src/vr/vr_input.cpp derives steering, throttle, brake and menu actions from headset controller state.

Sources:
- [Android Gradle configuration](https://github.com/SgtBilko76/SpeedDreams-VR/blob/master/android/app/build.gradle)
- [Android manifest](https://github.com/SgtBilko76/SpeedDreams-VR/blob/master/android/app/src/main/AndroidManifest.xml)
- [Native CMake configuration](https://github.com/SgtBilko76/SpeedDreams-VR/blob/master/CMakeLists.txt)
- [VR native entry point](https://github.com/SgtBilko76/SpeedDreams-VR/blob/master/src/vr/sd_vr.cpp)
- [VR input mapping](https://github.com/SgtBilko76/SpeedDreams-VR/blob/master/src/vr/vr_input.cpp)

## Why it cannot be shipped as a Redmi APK unchanged

1. The app's declared device capabilities and launch metadata are for VR hardware.
2. Native initialization expects an OpenXR runtime and an active XR session. A phone without a compatible headset runtime cannot satisfy this merely by hiding VR manifest entries.
3. Main-menu interactions and vehicle controls are sourced from headset-controller state. Phone touch events are not mapped into this system.
4. Removing the OpenXR frame loop requires a different surface/render/presentation path and mono view handling.
5. The source project references companion Speed Dreams source/data trees and third-party libraries that are not all committed in its own repository, so a clean CI build must reconstruct and pin those inputs.
6. The target's 2 GB RAM requires performance and memory measurements; an ARM64 compile does not prove acceptable gameplay.

## Required implementation

### A. Non-VR renderer and lifecycle

Create a phone-only backend using a normal Android EGL/GLES surface; decouple frame timing, camera transforms and presentation from OpenXR. Preserve Speed Dreams' physics, menus and modules where they do not depend on headset state. Keep the VR backend untouched if the upstream reference is reused, so the two paths can be tested independently.

### B. Touch and phone UI

Introduce explicit touch input events. Map menu tap/move to the engine's mouse events and map racing input to a virtual joystick: steering left/right, throttle, brake, reverse, gear shift, pause and camera. Make controls scalable and avoid obscuring the central driving view.

### C. Low-memory defaults

Use a conservative resolution/render scale, reduced texture size and draw distance, disable expensive visual effects by default, avoid preloading the entire car/track catalogue, and handle lifecycle memory pressure cleanly.

### D. Build, package and test

Pin source, dependencies and NDK; ensure reproducible native builds; inspect the output APK's manifest, package ID, ABI and minimum API. Then install and test the real app on Redmi A1+: startup, menu navigation, track load, touch driving, sound, pause/resume, completed race, peak memory and sustained frame rate.

## Release integrity

Do not rename, re-sign or relabel the VR APK and call it Redmi-compatible. Do not package a blank activity or a manifest-only hack as the game. Keep the APK release blocked until the non-OpenXR phone path is implemented and tested.

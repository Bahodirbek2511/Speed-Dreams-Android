# Android phone-port status

Last reviewed: 2026-10-26

## Current state

- Redmi-compatible phone APK: **not built**
- Native Android engine source available as a reference: **yes, through SpeedDreams-VR**
- Native Android build in this repository: **not attempted/successfully reproduced yet**
- Install/start/race tested on Redmi A1+: **no**
- The existing upstream portability audit: **passes**, but it does not compile the game or create an APK.

## Important discovery: an Android VR port already exists

[SpeedDreams-VR](https://github.com/SgtBilko76/SpeedDreams-VR) is a separate Android/ARM64 port of Speed Dreams 2.4. Its published v0.10.0-beta release includes Speeddreams.0.10.0-beta.apk (232,674,984 bytes, published 2026-10-20). It is designed for Meta Quest/Pico headsets, not ordinary Android phones.

Evidence from its source:
- android/app/build.gradle uses arm64-v8a, Android NDK 27.2.12479018, minSdk 29, and OpenXR.
- android/app/src/main/AndroidManifest.xml sets android.hardware.vr.headtracking to required="true", marks the app vr_only, and declares headset-specific launch categories/devices.
- CMakeLists.txt requires the OpenXR loader and compiles src/vr/sd_vr.cpp, whose native frame loop owns the OpenXR session.
- src/vr/vr_input.cpp maps steering/throttle/brake and menu interaction from headset controllers.

These facts make it a valuable engineering reference, but simply changing its package name or manifest does not create a working phone port.

## Work still required for Redmi A1+

1. Create a **non-OpenXR** Android surface/render loop using GLES and one on-screen camera view.
2. Route Speed Dreams drawing and game lifecycle to that surface without relying on an OpenXR runtime/session.
3. Add touch-driven menu mouse events and a virtual driving controller (steering, throttle, brake, gear/reverse, camera).
4. Select and pin a reproducible source/dependency set; the VR port relies on companion source/data and locally supplied third-party dependencies, so a clean CI build must first recreate that dependency layout.
5. Tune the graphics preset and memory footprint for 2 GB RAM; use compressed/lower-resolution textures and modest draw distance/effects.
6. Build/sign a real APK, inspect its ABI and manifest, then install and test on Redmi A1+ through a full race.

## Definition of done

The work is not complete until a phone-target APK installs on Android 12 Go, starts without OpenXR/headset services, displays the menus and track, accepts touch controls, completes a race, and has measured memory/frame-rate results on Redmi A1+.

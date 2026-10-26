# Phone-port gap analysis

Last reviewed: 2026-10-26

## Executive result

The repository now builds an experimental flat-screen Android APK using the public Speed Dreams engine and bundled game data. Earlier APKs were reported by the user as failing installation on Redmi A1+. The new candidate hardens the package identity, native ABIs, signing and GLES compatibility, but physical-device acceptance remains outstanding.

## Reference and adaptation

- Reference: [SgtBilko76/SpeedDreams-VR](https://github.com/SgtBilko76/SpeedDreams-VR), tag `v0.10.0-beta`.
- Engine: the public Speed Dreams `main` checkout, with the resolved commit recorded per build.
- Native dependencies are checked out from pinned refs and recorded in the source lock file.
- The patcher guards OpenXR-only startup, adds the phone EGL/GLES backend and its JNI lifecycle, integrates touch controls, adapts the public engine's screen/GUI setup and simplifies the race render loop to a single view.

The original VR APK is not a phone package: it declares headset-specific features, starts OpenXR and expects headset controllers. The phone workflow must use the patched build output, not the upstream VR APK unchanged.

## Reported installer failure and fixes

The previous APKs had a valid ZIP structure and a cryptographically valid v2 signature, but the earlier phone package retained upstream application identity, required GLES 3.1 even though the backend falls back to GLES 2, and used a newly generated CI debug signing key for each build. Those properties were unnecessarily fragile for sideload installation and repeated attempts.

The new candidate addresses those points:

1. `com.speeddreams.mobile` package identity avoids collisions with the VR reference and prior package name.
2. Both `arm64-v8a` and `armeabi-v7a` are included.
3. v1 and v2 APK signatures are explicitly enabled and checked.
4. A stable development signing key is used for subsequent test builds.
5. GLES 2.0 is declared as the minimum required GL feature, consistent with the phone EGL fallback.
6. CI checks APK metadata, alignment, ZIP integrity, signatures and the game-data archive.

These are evidence-based compatibility improvements, not a substitute for the user's device-level install test. If the new candidate fails, an `adb install` result is the next way to obtain the exact Package Manager failure code.

## Remaining required tests

1. Install the new Release APK on physical Redmi A1+.
2. Verify first-run all-files access handling and data extraction.
3. Launch the menu, load a track and complete a race.
4. Verify touch steering, throttle/brake, pause/camera and audio.
5. Record sustained FPS, memory, crashes, startup time and thermals.

A successful CI compile is necessary but does not prove the game runs correctly on the target phone.

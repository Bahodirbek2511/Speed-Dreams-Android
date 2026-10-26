# Build status

Last reviewed: 2026-10-26

## Current result

- Redmi-compatible Android APK: **not built**
- Android app compiled from this repository: **no**
- Redmi A1+ install/start/gameplay test: **not performed**
- Upstream portability audit: **successful** at [run 40293870115](https://github.com/Bahodirbek2511/Speed-Dreams-Android/actions/runs/40293870115)
- Existing Android/ARM64 engine-port reference: [SpeedDreams-VR](https://github.com/SgtBilko76/SpeedDreams-VR), but its app requires a VR headset and OpenXR; it is not a usable Redmi phone APK.

## Latest engineering findings

The VR reference provides an Android NDK build configuration (arm64-v8a, NDK 27.2.12479018, minSdk 29) and has already adapted a substantial portion of Speed Dreams to Android. However, it requires an OpenXR runtime, a VR-headtracking feature, headset metadata/categories, and controller input. Its build also depends on companion Speed Dreams source/data and third-party libraries that are not all committed directly to that repository.

A phone APK therefore needs a distinct non-OpenXR rendering and touch-input path, plus a reproducible dependency checkout/build before packaging. Stripping the VR manifest entries alone would only create an APK that may install but is not expected to start the game correctly.

## Build log

| Item | Result |
|---|---|
| Portability audit | PASS — run 40293870115 |
| Native Android build | NOT RUN |
| APK package | NOT CREATED |
| Redmi A1+ runtime test | NOT RUN |

When the first native build is attempted, record:
1. exact commit SHA and source/dependency revisions;
2. runner image, JDK, Android SDK/NDK and CMake versions;
3. exact build command;
4. result and first meaningful compiler/linker error;
5. APK artifact path, package ID, ABI, min SDK and signing mode.

Do not report a successful build unless the workflow completed and the produced APK was inspected.

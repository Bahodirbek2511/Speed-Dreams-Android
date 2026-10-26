# Build status

Last reviewed: 2026-10-26

## Current result

- Redmi-compatible Android APK: **not built**
- Android phone app compiled from this repository: **no**
- Redmi A1+ install/start/gameplay test: **not performed**
- Desktop upstream portability audit: **PASS**
- Android VR reference / phone-gap audit: **PASS** at [run 40294933235](https://github.com/Bahodirbek2511/Speed-Dreams-Android/actions/runs/40294933235)
- Audit artifact: [download both audit reports](https://github.com/Bahodirbek2511/Speed-Dreams-Android/actions/artifacts/12636939631)
- Existing Android/ARM64 engine-port reference: [SpeedDreams-VR](https://github.com/SgtBilko76/SpeedDreams-VR), but its app requires VR hardware and OpenXR; it is not a usable Redmi phone APK.

## Latest engineering findings

The VR reference provides an Android NDK build configuration (arm64-v8a, NDK 27.2.12479018, minSdk 29) and has adapted substantial Speed Dreams engine code to Android. Its manifest, native initialization/frame loop, and controls remain headset-specific. A phone APK needs a separate non-OpenXR rendering/input path.

The Android VR project also depends on companion Speed Dreams source/data and third-party libraries that are not all committed directly to its repository. A clean build must recreate and pin these inputs before a native compile can be meaningfully attempted.

## Build log

| Item | Result |
|---|---|
| Desktop upstream source audit | PASS |
| Android VR reference phone-gap audit | PASS |
| Native Android phone build | NOT RUN |
| Phone APK package | NOT CREATED |
| Redmi A1+ runtime test | NOT RUN |

## Next required engineering gates

1. Establish and pin all source/dependency checkouts.
2. Implement a phone-only GLES surface/frame loop with no OpenXR runtime dependency.
3. Implement touch-driven menu and driving controls.
4. Build the full native game and inspect the resulting phone APK.
5. Test install, startup, track load, input, sound, and a complete race on Redmi A1+.

A successful source audit is not a successful Android game build. Do not label any artifact as a playable phone APK until the native engine is packaged and device-tested.

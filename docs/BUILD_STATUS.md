# Build status

Last reviewed: 2026-10-26

## Current result

- Complete Redmi-compatible game APK: **not built**
- Complete Speed Dreams Android native engine build: **blocked / not run**
- Android ARM64 phone input/JNI library: **PASS** — [workflow run](https://github.com/Bahodirbek2511/Speed-Dreams-Android/actions/runs/40300397664)
- Input library artifact: [libsdphoneinput.so and exported-symbol report](https://github.com/Bahodirbek2511/Speed-Dreams-Android/actions/runs/40300397664/artifacts/12640157250)
- Host input tests and integration-patcher fixture tests: **PASS** — [workflow run](https://github.com/Bahodirbek2511/Speed-Dreams-Android/actions/runs/40300477268)
- Redmi A1+ installation/gameplay/performance test: **not performed**

The input/JNI artifact is a native control library only. It does not contain the Speed Dreams engine, tracks, cars, or a launchable game APK.

## Confirmed source blocker

The VR reference CMake project expects a companion Speed Dreams engine checkout on branch quest-port. The source probe found that the Forgejo engine remote responds but no longer advertises that branch. The probe recorded other branches but does not establish that they contain the Android patches needed by the VR reference.

- [Source probe workflow](https://github.com/Bahodirbek2511/Speed-Dreams-Android/actions/runs/40299728191)
- [Probe report artifact](https://github.com/Bahodirbek2511/Speed-Dreams-Android/actions/runs/40299728191/artifacts/12640256018)

Switching to the regular desktop main branch or merely removing OpenXR from the manifest would not prove the Android engine sources compile or run. The required Android engine changes and exact dependency revisions must be recovered or reimplemented first.

## Verified checks

| Check | Result |
|---|---|
| Classic upstream source portability audit | PASS |
| Android VR reference phone-gap audit | PASS |
| Phone touch-control host tests | PASS |
| Phone integration patcher tests on isolated fixtures | PASS |
| Android arm64-v8a input/JNI library compile and symbol inspection | PASS |
| Complete Speed Dreams Android engine build | BLOCKED |
| Full game APK package | NOT CREATED |
| Redmi A1+ runtime test | NOT RUN |

## Next required engineering gates

1. Reconstruct/pin the Android-compatible engine source and all dependency/data inputs.
2. Apply the phone backend to the real engine source and validate it against that source (fixture tests are not sufficient).
3. Build the full native engine with the Android NDK; fix actual compile/link/runtime errors.
4. Package the full game plus redistributable game data, inspect the APK manifest/ABI/native libraries.
5. Install on Redmi A1+, complete a race, and measure startup time, peak memory and sustained FPS.

A source audit or a compiled input library is not a completed game build. Do not label any artifact as a playable phone APK until the engine, renderer and gameplay are tested.

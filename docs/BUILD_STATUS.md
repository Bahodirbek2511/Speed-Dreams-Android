# Build status

Last reviewed: 2026-10-26

## Current result

- Complete Redmi-compatible game APK: **not built**
- Complete Speed Dreams Android native engine build: **blocked / not run**
- Android ARM64 input/JNI library: **PASS** — [build and inspection](https://github.com/Bahodirbek2511/Speed-Dreams-Android/actions/runs/40301485253)
- JNI library artifact from the earlier validated build: [libsdphoneinput.so and symbol report](https://github.com/Bahodirbek2511/Speed-Dreams-Android/actions/runs/40300397664/artifacts/12640157250)
- Android Java touch UI classes compiled against API 31: **PASS** — run [40301485253](https://github.com/Bahodirbek2511/Speed-Dreams-Android/actions/runs/40301485253)
- Host input tests including touch-queue overflow, stuck-key and mouse-button recovery: **PASS** — [run 40301485587](https://github.com/Bahodirbek2511/Speed-Dreams-Android/actions/runs/40301485587)
- Patcher fixture tests and source audits: **PASS** — [run 40300924001](https://github.com/Bahodirbek2511/Speed-Dreams-Android/actions/runs/40300924001)
- Redmi A1+ install/gameplay/performance test: **not performed**

The input/JNI artifact is a native control library only. It does not contain the Speed Dreams engine, tracks, cars, or a launchable game APK.

## Confirmed source blocker

The VR reference CMake project expects a companion Speed Dreams engine checkout on branch `quest-port`. The public Forgejo engine remote responds but does not advertise that branch. The source probe recorded other branches but did not establish that they contain the Android modifications required by the VR reference.

- [Required source branch probe](https://github.com/Bahodirbek2511/Speed-Dreams-Android/actions/runs/40299728191)
- [Public main source compatibility report](https://github.com/Bahodirbek2511/Speed-Dreams-Android/actions/runs/40300725640)
- [SDL screen initialization probe](https://github.com/Bahodirbek2511/Speed-Dreams-Android/actions/runs/40301327530)

The public engine `main` lacks the expected VR frame hooks. A phone port therefore needs real source changes around `guiscreen.cpp`, GUI frame presentation, and the race render loop, in addition to touch input. Merely removing OpenXR manifest entries is not a valid fix.

## Verified checks

| Check | Result |
|---|---|
| Desktop source portability audit | PASS |
| VR-reference phone-gap audit | PASS |
| Touch input / event queue host tests | PASS |
| Overflow recovery tests for held keys and mouse buttons | PASS |
| Patcher tests on isolated fixtures | PASS |
| Android ARM64 input/JNI library build and exported-symbol inspection | PASS |
| Android touch UI Java compile | PASS |
| Full Speed Dreams Android engine build | BLOCKED |
| Full game APK | NOT CREATED |
| Redmi A1+ runtime test | NOT RUN |

## Remaining engineering gates

1. Recover or reimplement a compatible Android engine source revision and pin dependencies.
2. Integrate a single phone-input implementation into the actual engine event loop.
3. Implement/verify Android EGL/GLES screen initialization, GUI frame presentation, and mono race rendering against the actual source tree.
4. Build the complete engine; fix actual compile/link errors.
5. Package the full game and legally distributable data, inspect APK metadata and libraries, then test a complete race on Redmi A1+.

A source audit and input-library compile do not constitute a playable game build.

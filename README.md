# Speed Dreams Android

Experimental workspace for a **native, flat-screen Android port** of Speed Dreams, targeting low-memory devices such as Redmi A1+ (2 GB RAM).

## Current status

**A playable Redmi phone APK has not been built.** The repository now has host-tested touch-control code and a separately built Android ARM64 input/JNI library, but these are not the Speed Dreams game itself.

A separate project, [SpeedDreams-VR](https://github.com/SgtBilko76/SpeedDreams-VR), has ported Speed Dreams 2.4 to Android ARM64 for standalone VR headsets. Its APK is **not compatible with Redmi A1+ as-is**: the manifest requires VR head-tracking hardware, native startup requires OpenXR, and controls expect headset controllers.

## Verified CI progress

- [Android ARM64 input/JNI library build](https://github.com/Bahodirbek2511/Speed-Dreams-Android/actions/runs/40300397664) — successful; artifact: [libsdphoneinput.so and symbol report](https://github.com/Bahodirbek2511/Speed-Dreams-Android/actions/runs/40300397664/artifacts/12640157250).
- [Touch-control unit tests and phone-patch fixture tests](https://github.com/Bahodirbek2511/Speed-Dreams-Android/actions/runs/40300477268) — successful.
- The source probe found a real blocker: the Speed Dreams engine remote does not advertise the VR port's required quest-port branch. [Probe run](https://github.com/Bahodirbek2511/Speed-Dreams-Android/actions/runs/40299728191) · [report artifact](https://github.com/Bahodirbek2511/Speed-Dreams-Android/actions/runs/40299728191/artifacts/12640256018).

These tests do **not** compile the complete game, package a phone APK, or prove Redmi gameplay compatibility.

## Target device

- Redmi A1+
- 2 GB RAM
- MediaTek Helio A22 / PowerVR GE8320
- Android 12 Go (device configuration may vary)

## Remaining engineering gates

1. Obtain or recreate a reproducible Android-compatible engine source revision, including the missing engine modifications and pinned dependencies.
2. Integrate one phone input implementation into the full engine without duplicate JNI/event/joystick symbols.
3. Build a non-OpenXR EGL/GLES phone renderer and single-view race path.
4. Package legally distributable game data, then build and inspect the complete arm64-v8a APK.
5. Install on the Redmi A1+, verify menus, track loading, touch controls, sound and a complete race, and measure memory/frame rate.

## Project documents

- [Porting plan](docs/PORTING_PLAN.md)
- [Build status](docs/BUILD_STATUS.md)
- [Android phone-port gap analysis](docs/PHONE_PORT_GAP_ANALYSIS.md)
- [Phone input component](porting/phone/README.md)
- [Experimental phone native backend](phone-port/README.md)

## Sources

- [Official Speed Dreams site](https://www.speed-dreams.net/)
- [Speed Dreams source](https://forge.a-lec.org/speed-dreams/speed-dreams-code/)
- [SpeedDreams-VR source](https://github.com/SgtBilko76/SpeedDreams-VR)
- [SpeedDreams-VR releases](https://github.com/SgtBilko76/SpeedDreams-VR/releases)

Never publish the VR APK, input-only library, or a placeholder app as the Redmi phone version. A release may be called playable only after the full native game has been tested on the actual device.

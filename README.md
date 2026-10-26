# Speed Dreams Android

Experimental workspace for a **native, flat-screen Android port** of Speed Dreams, targeting low-memory devices such as Redmi A1+ (2 GB RAM).

## Current status

**No Redmi-compatible phone APK has been built.** The existing GitHub Actions workflow only audits source portability.

A separate project, [SpeedDreams-VR](https://github.com/SgtBilko76/SpeedDreams-VR), has already ported Speed Dreams 2.4 to Android ARM64 for standalone VR headsets. This is useful technical reference, but it is **not a phone build**: its Android manifest requires VR head-tracking hardware, uses OpenXR, and expects headset controllers. Its APK must not be presented as compatible with Redmi A1+.

See [the phone-port gap analysis](docs/PHONE_PORT_GAP_ANALYSIS.md) for the evidence and required work.

## Target device

- Redmi A1+
- 2 GB RAM
- MediaTek Helio A22 / PowerVR GE8320
- Android 12 Go (device configuration may vary)

## Engineering goals

- Reuse proven Android/ARM64 dependency and engine-porting work where licensing and source provenance permit.
- Replace the headset-only OpenXR presentation path with a normal Android GLES surface and a mono camera/render path.
- Add touch controls for steering, throttle, brake, and menus.
- Keep rendering resolution, texture memory, and background workload low for a 2 GB device.
- Build a reproducible APK in GitHub Actions and keep full native build logs.
- Verify install, startup, track load, touch controls, and a complete race on the actual Redmi A1+ before claiming compatibility.

## Technical constraints

A manifest edit alone cannot turn the VR port into a phone game: its native startup and frame loop depend on an OpenXR runtime, while the controls are mapped from headset controllers. A phone version needs a real non-OpenXR rendering/input path. CI can verify a compile and package, but cannot prove playable performance on Redmi A1+ without device testing.

## Project documents

- [Porting plan](docs/PORTING_PLAN.md)
- [Build status](docs/BUILD_STATUS.md)
- [Android phone-port gap analysis](docs/PHONE_PORT_GAP_ANALYSIS.md)

## Sources

- [Official Speed Dreams site](https://www.speed-dreams.net/)
- [Speed Dreams source reference](https://github.com/argos-research/speed-dreams)
- [SpeedDreams-VR source](https://github.com/SgtBilko76/SpeedDreams-VR)
- [SpeedDreams-VR releases](https://github.com/SgtBilko76/SpeedDreams-VR/releases)

## Release integrity

Never publish a placeholder APK or a VR-only package as the Redmi phone version. Package game data only where its licence permits redistribution. Report exact build results and device-test evidence; do not infer phone compatibility from an ARM64 APK alone.

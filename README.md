# Speed Dreams Android

Experimental native, flat-screen Android port of Speed Dreams, with a low-memory device target such as Redmi A1+ (2 GB RAM).

## Current status

**An install-ready APK is being hardened; playability is not yet confirmed on a physical Redmi A1+.** Earlier APKs built successfully but the user-reported installer refused them with a “package appears invalid” message. The current fix revision addresses several compatibility weaknesses:

- separate phone application ID, so an upstream VR package or earlier build cannot collide with it;
- native libraries for both `arm64-v8a` and `armeabi-v7a`;
- explicit APK v1 and v2 signing plus a stable development certificate for repeat sideload updates;
- phone-specific GLES 2.0 manifest minimum, matching the EGL backend's ES3-to-ES2 fallback;
- automated checks for APK structure, package/version metadata, signing, alignment, bundled game-data archive and native ABIs.

The app is still experimental. A passing Android build and APK checks do not prove that the renderer, sound, controls or race loop work correctly on the target device.

## Verified build history

- [Previous universal ARM APK build and package inspection](https://github.com/Bahodirbek2511/Speed-Dreams-Android/actions/runs/40350874567) — compiled and packaged the engine plus bundled game data, but was followed by a report that installation failed. That candidate used v2-only signing and is superseded by the installer-hardening work.
- [Touch-control and patcher fixture tests](https://github.com/Bahodirbek2511/Speed-Dreams-Android/actions/runs/40300477268) — the host tests and transformations passed on their fixtures.
- Full engine/data inputs are recorded in each build's `android-source-lock.txt`; the CI bootstrap pins third-party dependency versions and records the resolved public engine commit.

## Target device

- Redmi A1+
- 2 GB RAM
- MediaTek Helio A22 / PowerVR GE8320
- Android 12 Go (device configuration can vary)

## Remaining acceptance gates

1. Install the hardened APK on the physical Redmi A1+ and confirm Package Installer accepts it.
2. Confirm first-launch storage permission/data extraction and the game menu.
3. Load a track and verify touchscreen steering, throttle, brake, pause/camera actions and sound.
4. Complete a race, then record crashes, memory use and sustained frame rate.

Until these device tests pass, the APK must be treated as an experimental build rather than a playable release.

## Project documents

- [Porting plan](docs/PORTING_PLAN.md)
- [Build status](docs/BUILD_STATUS.md)
- [Android port status](docs/ANDROID_PORT_STATUS.md)
- [Phone-port gap analysis](docs/PHONE_PORT_GAP_ANALYSIS.md)
- [Experimental phone native backend](phone-port/README.md)
- [Development signing-key warning](phone-port/signing/README.md)

## Sources

- [Official Speed Dreams site](https://www.speed-dreams.net/)
- [Speed Dreams source](https://forge.a-lec.org/speed-dreams/speed-dreams-code/)
- [SpeedDreams-VR source reference](https://github.com/SgtBilko76/SpeedDreams-VR)

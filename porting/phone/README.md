# Flat-screen Android input backend

This directory contains the first native components intended for the non-VR Speed Dreams Android port. They are platform-neutral C++ so the input and legacy GUI-event contract can be verified on CI without pretending that an Android game build has succeeded.

## Input mapping

| Input | Axis / bit |
|---|---|
| Steering | axis 0, -1 to +1, 8% deadzone |
| Throttle | axis 1, 0 to 1 |
| Brake / clutch | axis 4, 0 to 1 |
| Shift up | button bit 0 |
| Shift down | button bit 1 |
| ABS | button bit 2 |
| Reverse | button bit 4 |
| ASR | button bit 5 |
| Camera / pause / confirm / cancel | reserved button bits 6-9 |

`VrJoyRead(0, ...)` now adapts the phone's synchronized virtual-joystick state to the legacy input hook used by the Android engine reference. `SDPhoneInputPumpEvents()` converts queued tap, drag, release and cancel gestures into the GUI event ABI represented by `vr_events.h`, without importing OpenXR.

## Current integration status

- [x] Thread-safe steering/pedal/button state with finite-value clamping and steering deadzone.
- [x] Normalized touch-to-pixel menu events with bounded queue.
- [x] Engine-compatible virtual joystick adapter.
- [x] Mouse motion/down/up event translation for the legacy menu event loop.
- [x] CI host compilation and unit tests.
- [ ] JNI bridge and on-screen control view.
- [ ] Call the event pump from the phone-only native frame/event loop.
- [ ] Add a mono EGL/GLES lifecycle and remove OpenXR from the native startup path.
- [ ] Reproduce pinned engine/data/dependency checkouts and compile the full ARM64 engine.
- [ ] Package a real APK, inspect it, and test a complete race on Redmi A1+.

Passing the C++ tests validates only the input contract and adapter. It is not an Android APK build and does not establish gameplay or device compatibility.

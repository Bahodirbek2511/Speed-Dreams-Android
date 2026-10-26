# Flat-screen Android input contract

This directory contains the first native component intended for the non-VR Speed Dreams Android port. It is platform-neutral C++ so it can be unit-tested on CI without pretending that an Android game build has succeeded.

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

SDPhoneInputRead(0, ...) exposes the virtual joystick layout used by the Android VR engine port, without linking the phone input state to OpenXR.

Menu pointer events are queued in normalized coordinates and converted to pixels using the latest viewport size. The queue is protected because Android UI input and the engine loop run on different threads.

## Current integration status

- [x] Native, thread-safe steering/pedal/button state.
- [x] Normalized touch-to-pixel menu event queue.
- [x] Host-side unit tests.
- [ ] JNI bridge and on-screen control view.
- [ ] Route queued touch events into the Speed Dreams GUI event loop.
- [ ] Add a mono EGL/GLES frame loop and replace OpenXR lifecycle calls.
- [ ] Build the full engine and game data into an ARM64 phone APK.
- [ ] Install and complete a race on Redmi A1+.

Passing these unit tests validates only the input contract. It is not an Android APK build and does not establish gameplay or device compatibility.

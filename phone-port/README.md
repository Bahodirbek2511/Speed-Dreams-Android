# Android phone-port layer

This directory contains the first self-contained, host-tested portion of the flat-screen Android port.

## Current contents

- `src/phone_controls.*`: thread-safe multi-touch handling, menu pointer events, virtual steering/throttle/brake, gear/ABS/ASR bits, and edge-triggered pause/camera actions.
- `tests/phone_controls_test.cpp`: tests menu taps, racing mode transitions, simultaneous controls, cancellation, action edges, and repeated taps beyond the pointer-slot count.
- `tests/run_tests.sh`: compile and run the platform-independent test with warnings treated as errors.

Run from the repository root:

```sh
bash phone-port/tests/run_tests.sh
```

The controller module itself is not a game and does not produce an APK. It must be wired into the Android native backend, then integrated with the Speed Dreams engine and built against the reference port's pinned native dependencies. The final phone APK remains unverified until a full native build and real-device race test pass.

# Experimental Android phone-port backend

This directory contains an experimental flat-screen backend and a patch script targeting a clean checkout of [SpeedDreams-VR](https://github.com/SgtBilko76/SpeedDreams-VR). It is **not currently a completed game port**.

## Contents

- src/phone_controls.*: thread-safe multi-touch handling, menu pointer events, virtual steering/throttle/brake, gear/ABS/ASR bits, and edge-triggered pause/camera actions.
- src/phone_input_adapter.*: connects this input state to the VR reference's engine event interface in a single integrated native library.
- src/phone_stereo.cpp: draft mono-view implementation for the phone.
- src/sd_phone.cpp: draft Android EGL/GLES surface and JNI lifecycle without OpenXR.
- tools/apply_phone_port.py: fail-closed patcher for a local reference checkout; it refuses source layouts that do not match its tested assumptions.
- tests/phone_controls_test.cpp: verifies menu events, input mode transitions, multi-touch pedals/steering, cancellation and repeated taps.
- tests/test_apply_phone_port.py: exercises patcher transformations and idempotence on isolated fixture files, not on a real Speed Dreams engine tree.

## Tests

Run from the repository root:

```sh
bash phone-port/tests/run_tests.sh
python3 -m py_compile phone-port/tools/apply_phone_port.py phone-port/tests/test_apply_phone_port.py
python3 phone-port/tests/test_apply_phone_port.py
```

The host tests and fixture-based patcher tests pass in [CI run](https://github.com/Bahodirbek2511/Speed-Dreams-Android/actions/runs/40300477268).

## Current status and limitations

- GitHub Actions now bootstraps the pinned public Speed Dreams engine/data, compiles the phone backend, bundles the game data, signs the APK and verifies package metadata, signatures, ZIP integrity and both ARM native-library sets.
- The phone backend is experimental. A successful CI build proves that packaging and static checks passed; it does **not** prove the renderer, audio, controls or full race loop work correctly on a physical phone.
- The Android app uses a phone-specific application ID and a GLES 2.0 manifest minimum because its EGL initialization explicitly falls back from GLES 3 to GLES 2.
- The reproducible sideload build uses a public development signing key in `signing/`. It is not suitable for Google Play or production release; see [the signing-key warning](signing/README.md).
- The separate `porting/phone` directory is another independently tested input/UI prototype. Do not link both prototypes into one engine build: they contain overlapping JNI/joystick/event symbols.

A final installation and runtime check on Redmi A1+ is still required.

# Flat-screen Android input component

This directory contains the independently testable input/UI component for the non-VR Speed Dreams phone port. It is **not yet wired into a complete game renderer or APK**.

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

## Implemented and tested

- Thread-safe steering/pedal/button state with finite-value clamping and steering deadzone.
- Bounded normalized touch-to-pixel menu event queue.
- C++ engine adapter and JNI entry points.
- Android Canvas control overlay with multi-touch zones.
- Host C++ unit tests.
- Android arm64-v8a native shared-library build and exported-symbol checks: [CI run](https://github.com/Bahodirbek2511/Speed-Dreams-Android/actions/runs/40300397664), [artifact](https://github.com/Bahodirbek2511/Speed-Dreams-Android/actions/runs/40300397664/artifacts/12640157250).

The artifact is libsdphoneinput.so, a **control/input library only**. It does not include the Speed Dreams engine or constitute a game APK.

## Integration still required

- Integrate this input state with the full engine without duplicate VrJoyRead/VrQueue definitions.
- Call SDPhoneInputPumpEvents() from the actual engine event loop.
- Add/verify the non-OpenXR phone EGL/GLES lifecycle and mono renderer.
- Build the complete engine and package legally redistributable game data.
- Install, launch, load a track and complete a race on Redmi A1+.

A passing input-library build validates JNI/ABI integration for this component only; it does not establish game compatibility.

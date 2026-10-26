# Flat-screen Android input component

This directory contains an independently testable input/UI component for the non-VR Speed Dreams phone port. It is **not yet wired into the complete game renderer or APK**.

## Input mapping

| Input | Axis / bit |
|---|---|
| Steering | axis 0, -1 to +1, with 8% deadzone |
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
- When the touch queue overflows, it is reset with a synthetic CANCEL event rather than silently dropping an UP/CANCEL that could leave a press latched.
- GUI event queue overflow recovery: discard stale events, synthesize key-up and mouse-button-up for held inputs, then continue with the current event.
- Reset logic now releases input already consumed by the engine instead of only clearing pending events.
- C++ engine adapter and JNI entry points.
- Android Canvas control overlay.
- Host regression tests for sequential input, overflow handling, held-key release and mouse-button release: [run 40301485587](https://github.com/Bahodirbek2511/Speed-Dreams-Android/actions/runs/40301485587).
- Android arm64-v8a native shared-library build and symbol inspection, plus Java touch UI compilation against API 31: [run 40301485253](https://github.com/Bahodirbek2511/Speed-Dreams-Android/actions/runs/40301485253).

The native library artifact is a **control/input library only**. It does not include the Speed Dreams engine or constitute a game APK.

## Integration still required

- Integrate this input state with the full engine without duplicate `VrJoyRead` / `VrQueue*` definitions.
- Call `SDPhoneInputPumpEvents()` from the actual engine event loop.
- Implement/verify the non-OpenXR phone EGL/GLES lifecycle, GUI swap/present path and mono race renderer.
- Reconstruct/pin the Android-compatible engine source; the referenced `quest-port` branch is not advertised by the currently public remote.
- Build the complete engine, package legally redistributable game data, then install and complete a race on Redmi A1+.

Passing the input-library build validates this component only; it does not establish game compatibility.

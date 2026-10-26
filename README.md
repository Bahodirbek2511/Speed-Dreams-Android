# Speed Dreams Android

Experimental workspace for investigating an Android ARM64 port of Speed Dreams, targeting low-memory devices such as Redmi A1+ (2 GB RAM).

## Status

**Experimental — no working APK is available yet.** This repository tracks porting work; it does not contain the upstream game source and does not claim Android builds successfully.

## Goals

- Identify a compatible upstream revision and review source/data licensing.
- Audit native dependencies and graphics requirements for Android/ARM64.
- Establish a reproducible Android NDK build in GitHub Actions.
- Add Android input, lifecycle, storage, and packaging integration.
- Produce a debug APK only after a real native build succeeds.
- Measure startup, memory use, and frame rate on low-end hardware before claiming Redmi A1+ compatibility.

## Important constraints

Speed Dreams is a desktop C/C++ simulator with a substantial graphics and dependency stack. Android support is not a simple recompile. OpenSceneGraph/PLIB and other desktop assumptions may need replacement or significant porting. CI cannot prove playable performance on a Redmi A1+ without device testing.

## Project documents

- [Porting plan](docs/PORTING_PLAN.md)
- [Build status](docs/BUILD_STATUS.md)

## Upstream

- Website: https://www.speed-dreams.net/
- Source code: https://forge.a-lec.org/speed-dreams/speed-dreams-code/
- Source reference: https://github.com/argos-research/speed-dreams

## Contributing

Keep changes small and evidence-based. Never upload proprietary game assets. Record dependency versions, build commands, and actual errors. Do not publish a placeholder APK as if it were the game.

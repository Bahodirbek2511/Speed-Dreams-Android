# Android port status

## What is automated now

The Upstream portability audit GitHub Actions workflow checks out the upstream source and produces a report about its build system, desktop dependencies, and Android-specific files. It runs on relevant pushes, weekly, or manually.

## What it does not do

It does not compile Speed Dreams for Android and does not generate an APK. An APK containing only an empty activity or demo renderer would not be the requested game, so this repository will not label such a file as Speed Dreams.

## Technical blockers to resolve

1. Upstream Speed Dreams uses a legacy CMake build and has documented dependencies including SDL, PLIB, OpenAL, ENet, and optional OpenSceneGraph. Desktop libraries cannot be assumed to work with Android NDK.
2. Android requires a supported rendering path, native activity/lifecycle integration, touch controls, storage/audio handling, and packaging of the actual game assets.
3. Assets may have separate licenses. Only content whose license permits redistribution should be included.
4. The Redmi A1+ (2 GB RAM, PowerVR GE8320) needs real-device testing. A CI build alone cannot prove playable performance.

## How to run the audit

Open the repository Actions tab, choose Upstream portability audit, and press Run workflow. The generated Markdown report is attached to the completed workflow run as an artifact.

## Definition of done

The project is complete only when a real game APK is built, installs on the target Android version, starts the game, loads a track, accepts touch input, and completes a race without crashing. Performance and memory use must then be measured on the Redmi A1+.

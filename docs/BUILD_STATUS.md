# Build status

Last reviewed: 2026-10-26

## Current result

- Android APK: **not built**
- Android runtime: **not tested**
- Redmi A1+ compatibility: **not verified**
- Native dependency compatibility: **not yet audited**
- CI: repository scaffolding only until a real build workflow is added

## Known technical risk

The upstream game is a desktop C/C++ application with graphics and platform dependencies. A full Android port likely requires dependency work and changes to graphics, input, lifecycle, and packaging. Do not infer Android support from source code alone.

## Build log

Record each attempt with:
1. commit SHA and upstream revision;
2. runner image and NDK/CMake versions;
3. exact build command;
4. result and first meaningful error;
5. produced artifact path, if any.

Do not report a successful build unless the workflow actually completed successfully.

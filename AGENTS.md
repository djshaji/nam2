# Repository Guidelines

## Project Structure & Module Organization

NAM A2 is an Android live audio processor with a Java UI and C++20 DSP engine. `app/src/main/java/org/acoustixaudio/nam2/MainActivity.java` handles UI, permissions, and model import. `app/src/main/cpp/Engine.cpp` implements native audio and JNI integration; `Controls.h` contains gain, gate, and output controls. Native dependencies are configured in `app/src/main/cpp/CMakeLists.txt`. Standalone DSP checks live in `tests/controls_test.cpp`. No model captures are bundled; the UI is built programmatically.

## Build, Test, and Development Commands

Install JDK 17+ (21 recommended), Android SDK 36, NDK 28.2.13676358, and CMake 3.22.1. Set the SDK location through `local.properties` or `ANDROID_HOME`.

- `./gradlew :app:assembleDebug` builds the debug APK.
- `./gradlew :app:installDebug` installs it on a connected Android device or emulator.
- Run the standalone control checks without Android:

```sh
c++ -std=c++20 -Wall -Wextra -Iapp/src/main/cpp tests/controls_test.cpp -o /tmp/nam2-controls-test
/tmp/nam2-controls-test
```

Initial native configuration fetches Oboe 1.9.3 and NAM Core v0.5.2, including Eigen. For offline builds, supply complete checkouts through CMake's `FETCHCONTENT_SOURCE_DIR_OBOE` and `FETCHCONTENT_SOURCE_DIR_NAMCORE` settings.

## Coding Style & Naming Conventions

Follow surrounding Java and C++ code, using four-space indentation, PascalCase types, and camelCase methods and variables. Keep JNI declarations and implementations synchronized. No dedicated formatter or lint configuration is provided; native compilation enables `-Wall -Wextra`. Keep audio callbacks free of blocking work and allocations, preserve atomic parameter updates, and serialize model replacement with inference.

## Testing Guidelines

Tests use C++ `assert`; no coverage threshold is configured. Add DSP checks under `tests/` using the existing `*_test.cpp` naming pattern. Verify Android changes on a device with a wired/USB interface, covering model import, permission handling, background stops, and disconnects. Report build and hardware checks separately; standalone checks do not establish latency or inference performance.

## Commit & Pull Request Guidelines

Git history is unavailable in this checkout, so no existing commit convention can be confirmed. Use concise, imperative commit subjects. Pull requests should explain the behavior changed, link relevant issues, record verification and limitations, and include screenshots for UI changes.

## Configuration & Licensing

Keep SDK paths and generated build files untracked. Preserve fetched dependency licenses and notices, and respect each imported model's license.

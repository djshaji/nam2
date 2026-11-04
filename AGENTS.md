# Repository Guidelines

## Project Structure & Module Organization

NAM A2 is an Android live audio processor with a Java UI and C++20 DSP engine. `app/src/main/java/org/acoustixaudio/nam2/MainActivity.java` handles UI, permissions, and model import. `app/src/main/cpp/Engine.cpp` implements native audio and JNI integration; `Controls.h` contains gain, gate, and output controls. Native dependencies are configured in `app/src/main/cpp/CMakeLists.txt`. Standalone DSP checks live in `tests/controls_test.cpp`. No model captures are bundled; the UI is built programmatically.

`app/build.gradle.kts` defines Android SDK levels, supported ABIs, and native build settings. The app supports Android API 27+ and builds for `arm64-v8a` and `x86_64`. Keep user-facing setup and behavior documented in `README.md`.

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

The native build compiles NAM inference sources directly with `NAM_SAMPLE_FLOAT` and `NAM_ENABLE_A2_FAST`; upstream desktop tool targets are excluded. Preserve this separation when changing dependency configuration.

## Coding Style & Naming Conventions

Follow surrounding Java and C++ code, using four-space indentation, PascalCase types, and camelCase methods and variables. Keep JNI declarations and implementations synchronized. No dedicated formatter or lint configuration is provided; native compilation enables `-Wall -Wextra`. Keep audio callbacks free of blocking work and allocations, preserve atomic parameter updates, and serialize model replacement with inference.

## Audio and Lifecycle Constraints

- Serialize stream start/stop, model loading, and prewarming through the Java worker executor. Close streams before replacing the model; model import remains disabled during playback.
- Keep callback scratch storage fixed at 256 frames and split larger callbacks into blocks. Pass live gain, gate, and bypass changes through lock-free atomics.
- Accept mono NAM models only. Request the model's declared sample rate and reject negotiated rate mismatches; no resampler is implemented. Duplicate processed mono audio to stereo output.
- Preserve clean monitoring without a model and gain/gate processing during NAM bypass. Output defaults to −12 dB, sanitizes non-finite values, and clips to ±1.
- Import through the Android document picker with the existing 64 MB limit. Models remain loaded only for the current process session.
- Stop audio when the activity enters the background. Device errors signal the UI monitor; users restart playback after reconnecting. Routing uses the system defaults.

## Testing Guidelines

Tests use C++ `assert`; no coverage threshold is configured. Add DSP checks under `tests/` using the existing `*_test.cpp` naming pattern. Verify Android changes on a device with a wired/USB interface, covering model import, permission handling, background stops, and disconnects. Report build and hardware checks separately; standalone checks do not establish latency or inference performance.

For DSP changes, run the standalone checks with assertions enabled (do not define `NDEBUG`). For Java, JNI, or native build changes, attempt the debug APK build and report any SDK, dependency, or environment blockers explicitly. Device checks should also cover bypass, clean monitoring, and sample-rate mismatch errors. Do not claim measured latency or A2 inference performance without hardware evidence.

## Commit & Pull Request Guidelines

Use concise, imperative commit subjects. Pull requests should explain the behavior changed, link relevant issues, record verification and limitations, and include screenshots for UI changes.

## Configuration & Licensing

Keep SDK paths and generated build files untracked. Preserve fetched dependency licenses and notices, and respect each imported model's license.

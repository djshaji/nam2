# NAM A2 for Android

Initial native Android live effects processor using Oboe and NeuralAmpModelerCore v0.5.2 (A2 capable). No model captures are bundled.

## Build

Open this folder in Android Studio with JDK 17+ (JDK 21 recommended), Android SDK 36, NDK 28.2.13676358 and CMake 3.22.1 installed. Set the SDK path in local.properties or ANDROID_HOME, then run:

```sh
./gradlew :app:assembleDebug
```

The first native configure needs Git/network access to fetch Oboe 1.9.3 and NAM Core v0.5.2, including its Eigen submodule. Offline native builds can set CMake FETCHCONTENT_SOURCE_DIR_OBOE and FETCHCONTENT_SOURCE_DIR_NAMCORE to complete local checkouts. Upstream NAM's desktop tool targets are deliberately excluded.

## Use

Connect a wired/USB interface with instrument input and headphones. Grant microphone access, import a mono `.nam` capture through the Android document picker, and press Start audio. Input and output gain, a simple envelope noise gate, and NAM bypass can change during playback. Output starts at −12 dB and is hard clipped to ±1. Bypass skips NAM while retaining gate and gain. Without a model the app provides clean monitoring.

The app requests low latency exclusive streams; Oboe/device policy may negotiate shared access. The model's declared sample rate is requested and verified against the negotiated stream rate. No resampler is included. Mono captures only; stereo output duplicates the processed mono signal. Model import is limited to 64 MB and disabled during playback. Import and prewarm run on a serial worker; the callback uses fixed 256-frame scratch blocks and atomic controls. Model replacement never runs concurrently with inference.

Backgrounding stops audio. Device errors stop playback through the UI monitor; press Start to reopen streams after reconnecting. This version does not include a background service, IR cabinet convolution, presets, recording, input-channel selection, or automatic reconnection. Imported models are held in memory for this session and must be reimported after process restart. It uses the system's default input/output routes.

## Verification

The standalone DSP control checks can run without Android:

```sh
c++ -std=c++20 -Wall -Wextra -Iapp/src/main/cpp tests/controls_test.cpp -o /tmp/nam2-controls-test
/tmp/nam2-controls-test
```

Current environment: DSP checks passed, and MainActivity compiles against Android SDK 36. APK/native compilation is **not verified**: Gradle cannot create its network lock service in this sandbox, and shell DNS access to GitHub is blocked. Hardware latency, stream negotiation, A2 inference performance, underruns, and disconnect behavior still require device testing with a real capture/interface. No measured latency claim is made.

## Upstream references

- [NAM A2 release / Core v0.5.2](https://www.neuralampmodeler.com/post/a2-is-released)
- [NeuralAmpModelerCore](https://github.com/sdatkinson/NeuralAmpModelerCore/tree/v0.5.2)
- [Oboe full duplex guidance](https://github.com/google/oboe/blob/main/include/oboe/FullDuplexStream.h)

Preserve the fetched dependencies' licenses/notices when distributing the app, and follow the license of each imported model.

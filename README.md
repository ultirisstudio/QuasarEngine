# QuasarEngine

[![Windows](https://github.com/ultirisstudio/QuasarEngine/actions/workflows/windows.yml/badge.svg)](https://github.com/ultirisstudio/QuasarEngine/actions/workflows/windows.yml)
[![Linux](https://github.com/ultirisstudio/QuasarEngine/actions/workflows/linux.yml/badge.svg)](https://github.com/ultirisstudio/QuasarEngine/actions/workflows/linux.yml)
[![Android](https://github.com/ultirisstudio/QuasarEngine/actions/workflows/android.yml/badge.svg)](https://github.com/ultirisstudio/QuasarEngine/actions/workflows/android.yml)
[![Documentation](https://github.com/ultirisstudio/QuasarEngine/actions/workflows/docs.yml/badge.svg)](https://ultirisstudio.github.io/QuasarEngine/)

![Header files](https://img.shields.io/endpoint?url=https://ultirisstudio.github.io/QuasarEngine/badges/headers.json)
![Source files](https://img.shields.io/endpoint?url=https://ultirisstudio.github.io/QuasarEngine/badges/sources.json)
![Lines of code](https://img.shields.io/endpoint?url=https://ultirisstudio.github.io/QuasarEngine/badges/lines.json)
![Unit tests](https://img.shields.io/endpoint?url=https://ultirisstudio.github.io/QuasarEngine/badges/tests.json)
[![Line coverage](https://img.shields.io/endpoint?url=https://ultirisstudio.github.io/QuasarEngine/badges/coverage-lines.json)](https://ultirisstudio.github.io/QuasarEngine/coverage/)
[![Function coverage](https://img.shields.io/endpoint?url=https://ultirisstudio.github.io/QuasarEngine/badges/coverage-functions.json)](https://ultirisstudio.github.io/QuasarEngine/coverage/)
[![Condition coverage](https://img.shields.io/endpoint?url=https://ultirisstudio.github.io/QuasarEngine/badges/coverage-conditions.json)](https://ultirisstudio.github.io/QuasarEngine/coverage/)

A cross-platform 3D engine written in C++20, with an integrated editor. It uses OpenGL 4.5 or Vulkan 1.3 for rendering, EnTT for its entity component system and Dear ImGui for the editor interface.

The project is in early development and not usable yet. The API reference is published at
https://ultirisstudio.github.io/QuasarEngine/, and the coverage of the unit tests at
https://ultirisstudio.github.io/QuasarEngine/coverage/.

## Platforms

| Platform | Compiler |
|---|---|
| Windows 10/11 x64 | MSVC (Visual Studio 2022 or later) |
| Linux x64 | GCC 12+ or Clang 15+ |
| Android 13+ arm64 (sandbox only, Vulkan 1.3) | Clang of the Android NDK |

## Requirements

- Git
- CMake 3.28 or later
- On Windows: Visual Studio 2022 or later, with the "Desktop development with C++" workload
- Optional: the [Vulkan SDK](https://vulkan.lunarg.com/sdk/home) 1.3 or later, to build the Vulkan graphics API. Its
  installer sets the `VULKAN_SDK` environment variable, which CMake uses to find it. Without it, the engine only draws
  with OpenGL. On Linux, the `libvulkan-dev` package also works.
- On Linux: Ninja and the development packages used by SDL3 and OpenGL. On Ubuntu:

```sh
  sudo apt install ninja-build pkg-config libx11-dev libxrandr-dev libxcursor-dev libxi-dev \
      libxext-dev libxfixes-dev libxss-dev libxtst-dev libwayland-dev libxkbcommon-dev wayland-protocols \
      libdecor-0-dev libegl-dev libudev-dev libdbus-1-dev libgl1-mesa-dev
```

## Build

The dependencies are Git submodules, so clone with `--recursive`:

```sh
git clone --recursive https://github.com/ultirisstudio/QuasarEngine.git
cd QuasarEngine
```

If you already cloned without it, run `git submodule update --init --recursive`.

Then configure and build with a preset:

```sh
cmake --preset windows-debug
cmake --build --preset windows-debug
```

| Preset | Platform |
|---|---|
| `windows-debug`, `windows-release` | Windows, MSVC |
| `linux-gcc-debug`, `linux-gcc-release` | Linux, GCC |
| `linux-clang-debug`, `linux-clang-release` | Linux, Clang |

The first build takes a few minutes because every dependency is compiled. The programs are in
`build/<preset>/bin/<Debug or Release>/`:

- `QuasarEditor`: the editor
- `QuasarSandbox`: a small application to try the engine
- `QuasarTests`: the unit tests

Visual Studio and VS Code (with the CMake Tools extension) can also open the folder directly and use the same presets.

## Tests

```sh
ctest --preset windows-debug
```

## Android

The sandbox can be built as an APK. This needs JDK 17 and the Android SDK, with the SDK Platform 35, the NDK
28.2.13676358 and CMake 3.31.6, all installed from the SDK Manager of Android Studio. Set the `ANDROID_HOME`
environment variable to the folder of the SDK, or open the `android` folder in Android Studio once.

Build the APK from the `android` folder (on Windows, `gradlew.bat`):

```sh
cd android
./gradlew assembleDebug
```

Then install it on a device with USB debugging enabled, and follow its logs:

```sh
adb install -r app/build/outputs/apk/debug/app-debug.apk
adb logcat -s QuasarEngine SDL
```

`adb` is in the `platform-tools` folder of the SDK.

The sandbox runs in fullscreen, which hides the system bars until a swipe from an edge of the screen, and its Events
window can turn it off. It follows the rotation of the screen, unless the rotation is locked in the settings. Gamepads
work with a cable or over Bluetooth. A Steam Controller must first be paired in the Bluetooth settings of the device,
and the sandbox asks for the Bluetooth permission when it starts.

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md).

## License

QuasarEngine is licensed under the [Apache License 2.0](LICENSE).

# QuasarEngine

[![Windows](https://github.com/ultirisstudio/QuasarEngine/actions/workflows/windows.yml/badge.svg)](https://github.com/ultirisstudio/QuasarEngine/actions/workflows/windows.yml)
[![Linux](https://github.com/ultirisstudio/QuasarEngine/actions/workflows/linux.yml/badge.svg)](https://github.com/ultirisstudio/QuasarEngine/actions/workflows/linux.yml)
[![Documentation](https://github.com/ultirisstudio/QuasarEngine/actions/workflows/docs.yml/badge.svg)](https://ultirisstudio.github.io/QuasarEngine/)

A cross-platform 3D engine written in C++20, with an integrated editor. It uses OpenGL 4.5 for rendering, EnTT for its entity component system and Dear ImGui for the editor interface.

The project is in early development and not usable yet. The API reference is published at
https://ultirisstudio.github.io/QuasarEngine/.

## Platforms

| Platform | Compiler |
|---|---|
| Windows 10/11 x64 | MSVC (Visual Studio 2022 or later) |
| Linux x64 | GCC 12+ or Clang 15+ |

## Requirements

- Git
- CMake 3.28 or later
- On Windows: Visual Studio 2022 or later, with the "Desktop development with C++" workload
- On Linux: Ninja and the development packages used by GLFW, OpenGL and the file dialogs. On Ubuntu:

```sh
  sudo apt install ninja-build pkg-config libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev \
      libwayland-dev libxkbcommon-dev wayland-protocols libgl1-mesa-dev libgtk-3-dev
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

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md).

## License

QuasarEngine is licensed under the [Apache License 2.0](LICENSE).

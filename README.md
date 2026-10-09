# QuasarEngine

A cross-platform 3D engine written in C++20, with an integrated editor. It uses OpenGL 4.5 for rendering, EnTT for its entity component system and Dear ImGui for the editor interface.

The project is in early development and not usable yet.

## Platforms

| Platform | Compiler |
|---|---|
| Windows 10/11 x64 | MSVC (Visual Studio 2022 or later) |
| Linux x64 | GCC 12+ or Clang 15+ |

## Features

Longer term: animation playback, physically based rendering, physics, scripting and audio.

- [ ] Build system
  - [ ] CMake project with presets
  - [ ] Continuous integration on Windows and Linux
  - [ ] Unit tests
- [ ] Core
  - [ ] Logging
  - [ ] Window and OpenGL context
    - [ ] Events and input
    - [ ] ImGui integration
  - [ ] Layers and application loop
- [ ] Rendering
  - [ ] Buffers, shaders and textures
    - [ ] Framebuffers
    - [ ] Meshes and cameras
      - [ ] Materials and lighting
        - [ ] Normal mapping
        - [ ] Shadows
      - [ ] Skybox
- [ ] Scene
  - [ ] Entities and components
    - [ ] Parent and child hierarchy
    - [ ] Saving and loading
- [ ] Assets
  - [ ] Asset registry with `.meta` files
    - [ ] Texture import
    - [ ] Model import (FBX, glTF, OBJ)
      - [ ] Material import
      - [ ] Skeleton and animation clip import
    - [ ] Reimport that keeps references valid
- [ ] Projects
  - [ ] Create and open projects
  - [ ] Recent projects
- [ ] Editor
  - [ ] Dockable layout with viewport and console
    - [ ] Hierarchy and inspector
      - [ ] Gizmos and object picking
    - [ ] Content browser
      - [ ] Asset inspector and reimport

## License

QuasarEngine is licensed under the [Apache License 2.0](LICENSE).

# QuasarEngine

A cross-platform 3D engine written in C++20, with an integrated editor. It uses OpenGL 4.5 for rendering, EnTT for its entity component system and Dear ImGui for the editor interface.

The project is in early development and not usable yet.

## Platforms

| Platform | Compilers |
|---|---|
| Windows 10/11 x64 | MSVC (Visual Studio 2022) |
| Linux x64 | GCC 12+, Clang 15+ |

## Features

Planned for the first version:

- [ ] Window, input and event system
- [ ] OpenGL 4.5 renderer
- [ ] Materials and lighting
- [ ] Skybox
- [ ] Entity component system (EnTT)
- [ ] Scene hierarchy, saving and loading
- [ ] Asset pipeline with `.meta` files
- [ ] Texture import
- [ ] 3D model import (FBX, glTF, OBJ)
- [ ] Skeleton and animation clip import
- [ ] Dockable editor with viewport, hierarchy and inspector
- [ ] Gizmos and object picking
- [ ] Content browser
- [ ] Project creation and management

Planned later:

- [ ] Animation playback and skinning
- [ ] Shadows
- [ ] Physically based rendering
- [ ] Scripting
- [ ] Physics
- [ ] Audio

## Contributing

Contributions are welcome. All tasks are tracked as issues in the organization's GitHub Project.

1. Pick a task in the **Ready** column that nobody is assigned to. If you are new to the project, start with one labeled `good first issue`.
2. Claim it: team members assign themselves, other contributors leave a comment and wait for a maintainer to assign them.
3. Work on a branch named after the task, for example `feature/CORE-15-events`. External contributors work from a fork.
4. Open a pull request that includes `Closes #<issue number>`. CI must pass on Windows and Linux, and one approval is required before merging.

To report a bug or propose a task, open an issue with the matching template.

## License

QuasarEngine is licensed under the [Apache License 2.0](LICENSE).

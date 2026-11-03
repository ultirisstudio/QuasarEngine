# Contributing to QuasarEngine

Read the [README](README.md) first to build the project and run the tests.

## Workflow

1. Pick an issue from the project board, or open one before starting a large change.
2. Create a branch from an up-to-date `master`.
3. Open a pull request and fill in the template. Link the issue with `Closes #N`.
4. The pull request is squash merged once the checks pass and the review is done.

Everything in the repository is written in English: code, comments, commit messages, issues and pull requests.

## Before you push

Every pull request must pass these checks. Run them locally first:

| Check | Command |
|---|---|
| Build | `cmake --build --preset <preset>` |
| Unit tests | `ctest --preset <preset>` |
| Formatting | `clang-format --dry-run --Werror <file>` |
| Static analysis | `run-clang-tidy -p build/<preset> '/(engine\|sandbox\|editor\|tests)/src/'` |
| Documentation | `doxygen docs/Doxyfile` |

Warnings are treated as errors. The CI builds on Windows with MSVC and on Linux with GCC and Clang, in Debug and
Release, so code that builds on your machine can still fail on another compiler.

## Sanitizers

The sanitizers add checks to a program while it runs. The program stops with a report on the first memory error,
undefined behavior or data race, and lists the memory leaks when it exits. The CI runs the unit tests with them.

| Preset | Checks |
|---|---|
| `windows-asan` | Memory errors |
| `linux-gcc-asan`, `linux-clang-asan` | Memory errors, memory leaks and undefined behavior |
| `linux-gcc-tsan`, `linux-clang-tsan` | Data races and undefined behavior |

To check the sandbox or the editor, build one of these presets and run the program as usual. For example, on Linux:

```sh
cmake --preset linux-clang-asan
cmake --build --preset linux-clang-asan
ctest --preset linux-clang-asan
build/linux-clang-asan/bin/Debug/QuasarSandbox
```

On Windows, use the `windows-asan` preset the same way, or select it in Visual Studio. The programs are in
`build/windows-asan/bin/Debug`. When a check fails under the Visual Studio debugger, it stops on the faulty line.

On Linux, if a program stops at startup with an error about memory mappings, run `sudo sysctl vm.mmap_rnd_bits=28`.

## Code style

The formatting is defined in `.clang-format`. Use clang-format 18, like the CI (`pip install clang-format==18.1.8`),
and format the files you change with `clang-format -i <file>` or with format on save in your editor.

- All engine code is in the `Quasar` namespace.
- Types and functions use PascalCase. Members start with `m_`, static members with `s_`.
- Public headers go in `engine/include/quasar/`, source files in `engine/src/`. Include engine headers with
  `#include <quasar/...>`.
- New engine files must be added to `target_sources` in `engine/CMakeLists.txt`.
- Errors are reported with `std::optional` or `bool`, and a `QUASAR_ENGINE_ERROR` message. Do not throw exceptions.

## Static analysis

clang-tidy 18 reads the code without running it and reports bugs and risky code, such as a use after `std::move`, an
empty `std::optional` that is read, or a name that does not follow the code style. The checks are listed in
`.clang-tidy`, and `tests/.clang-tidy` changes a few of them for the tests. Every report is an error.

On Linux, configure a preset, then run `run-clang-tidy` from the repository root (see the table above). In Visual
Studio, use Analyze > Run Code Analysis.

When a report is wrong for one line, explain why in a comment, then silence it for that line only:

```cpp
// The only application, reached from anywhere with Application::Get()
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
Application* s_Instance = nullptr;
```

## Tests

Tests use [doctest](https://github.com/doctest/doctest) and live in `tests/src/`, one file per class, for example
`UUIDTests.cpp`. New files in this folder are found automatically after the next CMake configure.

Each `TEST_CASE` becomes its own CTest test, so its name must not contain a comma or `*`. Write it as a short
sentence: `TEST_CASE("A default UUID is invalid")`.

### What to test

Test what the code does, not more and not less:

- Check results and behavior: return values, state, the events sent, the order of the calls. A test fails only when
  the behavior changes.
- Do not read the text of log messages to check something else. Check the level of the messages instead, with
  `TestHelpers::HasLogAtLeast()`: "no error was logged", "a warning was logged". The text of a message can be checked
  only when writing it is the job of the tested code, as for `ScopedTimer`.
- Do not test the standard library or the third-party libraries, for example that a `std::shared_ptr` counts its
  references.
- The same behavior is tested on Windows and Linux. A test is limited to one platform only for code that only exists
  on that platform.

### Test helpers

The helpers shared by the tests are in `tests/src/`:

| Header | Content |
|---|---|
| `LogHelpers.h` | `ScopedLog`, which starts and stops the loggers for one test, and `HasLogAtLeast()` |
| `NullWindow.h` | Windows and applications on the null platform of GLFW, and functions that send them input |
| `TestLayers.h` | `ClosingLayer`, which closes the application after a number of frames |
| `TemporaryFiles.h` | `TemporaryFolder`, `TemporaryFile` and `ScopedCurrentDirectory`, removed or put back at the end of the test |
| `ScopedEnvironmentVariable.h` | Changes an environment variable during a test (Linux only) |

## Documentation

The API reference is generated by Doxygen from the headers in `engine/include`. Every public element must be
documented, or the "Check documentation" job fails.

```cpp
/// \file
/// \brief Functions to read a whole file from the disk.
/// \author Kurogenshi
/// \date 2026-10-19

/// @brief Reads a whole binary file, byte for byte.
/// @param path Path of the file to read.
/// @return The bytes of the file, or no value if it cannot be read.
std::optional<std::vector<std::byte>> ReadBinaryFile(const std::filesystem::path& path);
```

- Each header starts with a `\file` comment.
- Each class, function, alias and macro has a `///` comment just before it.
- Fill in every `@param` and `@return`, or leave them out. An empty tag shows an empty section on the site.
- A line without `///` ends the comment, so separate paragraphs with an empty `///` line.
- Put code between backticks, for example `` `#ifdef` ``. Otherwise Doxygen can read it as a command.
- A macro defined differently per platform or build type is documented once with `/// \def NAME`, placed before the
  `#ifdef`.
- Private members and `.cpp` files use normal `//` comments.

To check the documentation locally, install Doxygen 1.18.0 and run from the repository root (the `build` folder
must exist):

```sh
git submodule update --init external/doxygen-awesome-css
doxygen docs/Doxyfile
```

It must print nothing. The site is generated in `build/docs/html/index.html`.
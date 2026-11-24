add_library(QuasarEditorLibrary STATIC)
add_library(Quasar::EditorLibrary ALIAS QuasarEditorLibrary)

target_sources(QuasarEditorLibrary PRIVATE
    src/EditorApplication.cpp
    src/EditorApplication.h
    src/EditorContext.h
    src/EditorLayer.cpp
    src/EditorLayer.h
    src/EditorPanel.cpp
    src/EditorPanel.h
    src/Project.cpp
    src/Project.h
    src/ProjectPopups.cpp
    src/ProjectPopups.h
    src/panels/ConsolePanel.cpp
    src/panels/ConsolePanel.h
    src/panels/GamepadPanel.cpp
    src/panels/GamepadPanel.h
)

target_include_directories(QuasarEditorLibrary PUBLIC src)

target_link_libraries(QuasarEditorLibrary
    PUBLIC
        Quasar::Engine
    PRIVATE
        Quasar::CompileOptions
)

set_target_properties(QuasarEditorLibrary PROPERTIES FOLDER "Editor")

add_executable(QuasarEditor)

target_sources(QuasarEditor PRIVATE
    src/editor.cpp
)

target_link_libraries(QuasarEditor PRIVATE
    Quasar::EditorLibrary
    Quasar::EntryPoint
    Quasar::CompileOptions
)

set_target_properties(QuasarEditor PROPERTIES FOLDER "Applications")

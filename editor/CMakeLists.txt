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
    src/EditorSettings.cpp
    src/EditorSettings.h
    src/EditorText.cpp
    src/EditorText.h
    src/EditorUI.cpp
    src/EditorUI.h
    src/Project.cpp
    src/Project.h
    src/ProjectPopups.cpp
    src/ProjectPopups.h
    src/panels/ContentBrowserPanel.cpp
    src/panels/ContentBrowserPanel.h
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
        yaml-cpp::yaml-cpp
)

add_custom_target(QuasarEditorResources
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different "${CMAKE_CURRENT_SOURCE_DIR}/resources" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/editor"
    VERBATIM
)
add_dependencies(QuasarEditorLibrary QuasarEditorResources)

set_target_properties(QuasarEditorLibrary QuasarEditorResources PROPERTIES FOLDER "Editor")

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

add_library(QuasarEngine STATIC)
add_library(Quasar::Engine ALIAS QuasarEngine)

target_sources(QuasarEngine
    PRIVATE
        src/Version.cpp
        src/core/Log.cpp
        src/core/UUID.cpp
        src/io/File.cpp
        src/core/Timer.cpp
        src/core/Window.cpp
        src/core/LayerStack.cpp
        src/core/ScopedTimer.cpp
        src/core/Application.cpp
        src/core/MemoryLogSink.cpp
        src/io/Path.cpp
        src/core/Input.cpp
        src/io/UserDataDirectory.cpp
        src/io/ExecutableDirectory.h
        src/io/ExecutableDirectory.cpp
        src/renderer/OpenGLContext.cpp
        src/renderer/OpenGLContext.h
        src/core/ImGuiLayer.cpp
        src/core/ImGuiTheme.cpp
        src/core/Gamepads.cpp
        src/core/SdlGamepadReader.h
        src/core/SdlGamepadReader.cpp
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS include
        FILES
            include/quasar/Version.h
            include/quasar/core/PlatformDetection.h
            include/quasar/core/Log.h
            include/quasar/core/Base.h
            include/quasar/core/UUID.h
            include/quasar/core/Timestep.h
            include/quasar/io/File.h
            include/quasar/core/Timer.h
            include/quasar/core/KeyCodes.h
            include/quasar/core/MouseCodes.h
            include/quasar/core/Window.h
            include/quasar/events/Event.h
            include/quasar/events/EventDispatcher.h
            include/quasar/events/WindowEvent.h
            include/quasar/events/KeyEvent.h
            include/quasar/events/MouseEvent.h
            include/quasar/core/Layer.h
            include/quasar/core/LayerStack.h
            include/quasar/core/ScopedTimer.h
            include/quasar/core/Application.h
            include/quasar/core/MemoryLogSink.h
            include/quasar/io/Path.h
            include/quasar/core/Input.h
            include/quasar/core/EntryPoint.h
            include/quasar/io/UserDataDirectory.h
            include/quasar/core/ImGuiLayer.h
            include/quasar/core/ImGuiTheme.h
            include/quasar/core/GamepadCodes.h
            include/quasar/core/Gamepads.h
            include/quasar/events/GamepadEvent.h
)

add_custom_target(QuasarEngineResources
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different "${CMAKE_CURRENT_SOURCE_DIR}/resources" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources"
    VERBATIM
)
set_target_properties(QuasarEngineResources PROPERTIES FOLDER "Engine")
add_dependencies(QuasarEngine QuasarEngineResources)

add_library(QuasarEntryPoint STATIC src/core/EntryPoint.cpp)
add_library(Quasar::EntryPoint ALIAS QuasarEntryPoint)
target_link_libraries(QuasarEntryPoint PUBLIC Quasar::Engine PRIVATE Quasar::CompileOptions)
set_target_properties(QuasarEntryPoint PROPERTIES FOLDER "Engine")

if(NOT WIN32 AND NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR "QuasarEngine only supports Windows and Linux.")
endif()

target_include_directories(QuasarEngine PRIVATE src)

target_link_libraries(QuasarEngine
    PUBLIC
        glm::glm
        spdlog::spdlog
        imgui::imgui
    PRIVATE
        Quasar::CompileOptions
        glfw
        SDL3::SDL3
        glad::glad
)

target_compile_definitions(QuasarEngine PRIVATE
    GLFW_INCLUDE_NONE
    QUASAR_VERSION="${PROJECT_VERSION}"
)

set_target_properties(QuasarEngine PROPERTIES FOLDER "Engine")

get_target_property(QUASAR_ENGINE_SOURCES QuasarEngine SOURCES)
get_target_property(QUASAR_ENGINE_HEADERS QuasarEngine HEADER_SET)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${QUASAR_ENGINE_SOURCES} ${QUASAR_ENGINE_HEADERS})
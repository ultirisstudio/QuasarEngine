add_library(QuasarEngine STATIC)
add_library(Quasar::Engine ALIAS QuasarEngine)

target_sources(QuasarEngine
    PRIVATE
        src/Version.cpp
        src/core/Application.cpp
        src/core/LayerStack.cpp
        src/core/UUID.cpp
        src/core/Window.cpp
        src/imgui/ImGuiBackend.cpp
        src/imgui/ImGuiBackend.h
        src/imgui/ImGuiLayer.cpp
        src/imgui/ImGuiTheme.cpp
        src/input/Gamepads.cpp
        src/input/Input.cpp
        src/input/SdlGamepadBackend.cpp
        src/input/SdlGamepadBackend.h
        src/io/ExecutableDirectory.cpp
        src/io/ExecutableDirectory.h
        src/io/File.cpp
        src/io/Path.cpp
        src/io/UserDataDirectory.cpp
        src/log/Log.cpp
        src/log/MemoryLogSink.cpp
        src/renderer/GraphicsContextFactory.cpp
        src/renderer/GraphicsContextFactory.h
        src/time/ScopedTimer.cpp
        src/time/Timer.cpp
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS include
        FILES
            include/quasar/Version.h
            include/quasar/core/Application.h
            include/quasar/core/Base.h
            include/quasar/core/EntryPoint.h
            include/quasar/core/Layer.h
            include/quasar/core/LayerStack.h
            include/quasar/core/PlatformDetection.h
            include/quasar/core/UUID.h
            include/quasar/core/Window.h
            include/quasar/events/Event.h
            include/quasar/events/EventDispatcher.h
            include/quasar/events/GamepadEvent.h
            include/quasar/events/KeyEvent.h
            include/quasar/events/MouseEvent.h
            include/quasar/events/WindowEvent.h
            include/quasar/imgui/ImGuiLayer.h
            include/quasar/imgui/ImGuiTheme.h
            include/quasar/input/GamepadBackend.h
            include/quasar/input/GamepadCodes.h
            include/quasar/input/GamepadState.h
            include/quasar/input/Gamepads.h
            include/quasar/input/Input.h
            include/quasar/input/KeyCodes.h
            include/quasar/input/MouseCodes.h
            include/quasar/io/File.h
            include/quasar/io/Path.h
            include/quasar/io/UserDataDirectory.h
            include/quasar/log/Log.h
            include/quasar/log/MemoryLogSink.h
            include/quasar/renderer/GraphicsAPI.h
            include/quasar/renderer/GraphicsContext.h
            include/quasar/time/ScopedTimer.h
            include/quasar/time/Timer.h
            include/quasar/time/Timestep.h
)

if(QUASAR_OPENGL)
    target_sources(QuasarEngine
        PRIVATE
            src/platform/opengl/OpenGLContext.cpp
            src/platform/opengl/OpenGLImGuiBackend.cpp
            src/platform/opengl/OpenGLImGuiBackend.h
        PUBLIC
            FILE_SET HEADERS
            FILES
                include/quasar/platform/opengl/OpenGLContext.h
    )
    target_link_libraries(QuasarEngine PRIVATE glad::glad)
    target_compile_definitions(QuasarEngine PRIVATE QUASAR_OPENGL)
endif()

if(QUASAR_VULKAN)
    target_sources(QuasarEngine
        PRIVATE
            src/platform/vulkan/VulkanContext.cpp
            src/platform/vulkan/VulkanImGuiBackend.cpp
            src/platform/vulkan/VulkanImGuiBackend.h
        PUBLIC
            FILE_SET HEADERS
            FILES
                include/quasar/platform/vulkan/VulkanContext.h
    )
    target_link_libraries(QuasarEngine PUBLIC Vulkan::Vulkan)
    target_compile_definitions(QuasarEngine PRIVATE QUASAR_VULKAN)
endif()

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
        SDL3::SDL3
)

target_compile_definitions(QuasarEngine PRIVATE
    QUASAR_VERSION="${PROJECT_VERSION}"
)

set_target_properties(QuasarEngine PROPERTIES FOLDER "Engine")

get_target_property(QUASAR_ENGINE_SOURCES QuasarEngine SOURCES)
get_target_property(QUASAR_ENGINE_HEADERS QuasarEngine HEADER_SET)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${QUASAR_ENGINE_SOURCES} ${QUASAR_ENGINE_HEADERS})

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
        src/io/EngineResources.cpp
        src/io/ExecutableDirectory.cpp
        src/io/ExecutableDirectory.h
        src/io/File.cpp
        src/io/FileDialog.cpp
        src/io/Path.cpp
        src/io/SdlFileDialogBackend.cpp
        src/io/SdlFileDialogBackend.h
        src/io/UserDataDirectory.cpp
        src/io/UserFolders.cpp
        src/log/Log.cpp
        src/log/MemoryLogSink.cpp
        src/localization/Localization.cpp
        src/localization/TextLayout.cpp
        src/platform/opengl/OpenGLBuffer.cpp
        src/platform/opengl/OpenGLBuffer.h
        src/platform/opengl/OpenGLContext.cpp
        src/platform/opengl/OpenGLDevice.cpp
        src/platform/opengl/OpenGLDevice.h
        src/platform/opengl/OpenGLFramebuffer.cpp
        src/platform/opengl/OpenGLFramebuffer.h
        src/platform/opengl/OpenGLPipeline.cpp
        src/platform/opengl/OpenGLPipeline.h
        src/platform/opengl/OpenGLShader.cpp
        src/platform/opengl/OpenGLShader.h
        src/platform/opengl/OpenGLState.cpp
        src/platform/opengl/OpenGLState.h
        src/platform/opengl/OpenGLTexture.cpp
        src/platform/opengl/OpenGLTexture.h
        src/platform/opengl/OpenGLTimestampQuery.cpp
        src/platform/opengl/OpenGLTimestampQuery.h
        src/platform/opengl/OpenGLVertexArray.cpp
        src/platform/opengl/OpenGLVertexArray.h
        src/platform/opengl/OpenGLImGuiBackend.cpp
        src/platform/opengl/OpenGLImGuiBackend.h
        src/renderer/AmbientOcclusion.cpp
        src/renderer/Bloom.cpp
        src/renderer/Buffer.cpp
        src/renderer/Camera.cpp
        src/renderer/Color.cpp
        src/renderer/Framebuffer.cpp
        src/renderer/FullscreenPass.cpp
        src/renderer/GpuProfiler.cpp
        src/renderer/GraphicsContext.cpp
        src/renderer/GraphicsContextFactory.cpp
        src/renderer/GraphicsContextFactory.h
        src/renderer/Image.cpp
        src/renderer/LightsBuffer.cpp
        src/renderer/LightsBuffer.h
        src/renderer/Material.cpp
        src/renderer/Mesh.cpp
        src/renderer/Pipeline.cpp
        src/renderer/PipelineLibrary.cpp
        src/renderer/PostProcessing.cpp
        src/renderer/Primitives.cpp
        src/renderer/Renderer.cpp
        src/renderer/Shader.cpp
        src/renderer/Shadows.cpp
        src/renderer/Texture.cpp
        src/renderer/ToneMapper.cpp
        src/renderer/UniformBlockLayout.cpp
        src/renderer/VertexArray.cpp
        src/renderer/VertexLayout.cpp
        src/scene/Entity.cpp
        src/scene/FlyCameraController.cpp
        src/scene/Scene.cpp
        src/scene/SceneRenderer.cpp
        src/scene/components/TransformComponent.cpp
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
            include/quasar/io/EngineResources.h
            include/quasar/io/File.h
            include/quasar/io/FileDialog.h
            include/quasar/io/Path.h
            include/quasar/io/UserDataDirectory.h
            include/quasar/io/UserFolders.h
            include/quasar/log/Log.h
            include/quasar/log/MemoryLogSink.h
            include/quasar/localization/Localization.h
            include/quasar/localization/TextLayout.h
            include/quasar/platform/opengl/OpenGLContext.h
            include/quasar/renderer/AmbientOcclusion.h
            include/quasar/renderer/Bloom.h
            include/quasar/renderer/Buffer.h
            include/quasar/renderer/Camera.h
            include/quasar/renderer/Color.h
            include/quasar/renderer/Framebuffer.h
            include/quasar/renderer/FullscreenPass.h
            include/quasar/renderer/GpuProfiler.h
            include/quasar/renderer/GraphicsAPI.h
            include/quasar/renderer/GraphicsContext.h
            include/quasar/renderer/Image.h
            include/quasar/renderer/Lights.h
            include/quasar/renderer/Material.h
            include/quasar/renderer/Mesh.h
            include/quasar/renderer/Pipeline.h
            include/quasar/renderer/PipelineLibrary.h
            include/quasar/renderer/PostProcessing.h
            include/quasar/renderer/Primitives.h
            include/quasar/renderer/RenderStatistics.h
            include/quasar/renderer/Renderer.h
            include/quasar/renderer/Shader.h
            include/quasar/renderer/ShaderBindings.h
            include/quasar/renderer/Shadows.h
            include/quasar/renderer/Texture.h
            include/quasar/renderer/TimestampQuery.h
            include/quasar/renderer/ToneMapper.h
            include/quasar/renderer/UniformBlockLayout.h
            include/quasar/renderer/VertexArray.h
            include/quasar/renderer/VertexLayout.h
            include/quasar/scene/Entity.h
            include/quasar/scene/FlyCameraController.h
            include/quasar/scene/Scene.h
            include/quasar/scene/SceneRenderer.h
            include/quasar/scene/components/CameraComponent.h
            include/quasar/scene/components/DirectionalLightComponent.h
            include/quasar/scene/components/IdComponent.h
            include/quasar/scene/components/MeshRendererComponent.h
            include/quasar/scene/components/NameComponent.h
            include/quasar/scene/components/PointLightComponent.h
            include/quasar/scene/components/RelationshipComponent.h
            include/quasar/scene/components/SpotLightComponent.h
            include/quasar/scene/components/TransformComponent.h
            include/quasar/time/ScopedTimer.h
            include/quasar/time/Timer.h
            include/quasar/time/Timestep.h
)

add_custom_target(QuasarEngineResources
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different "${CMAKE_CURRENT_SOURCE_DIR}/resources" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/engine"
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
        EnTT::EnTT
    PRIVATE
        Quasar::CompileOptions
        SDL3::SDL3
        glad::glad
        stb::image
        yaml-cpp::yaml-cpp
        SheenBidi::SheenBidi
)

target_compile_definitions(QuasarEngine PRIVATE
    QUASAR_VERSION="${PROJECT_VERSION}"
)

set_target_properties(QuasarEngine PROPERTIES FOLDER "Engine")

get_target_property(QUASAR_ENGINE_SOURCES QuasarEngine SOURCES)
get_target_property(QUASAR_ENGINE_HEADERS QuasarEngine HEADER_SET)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${QUASAR_ENGINE_SOURCES} ${QUASAR_ENGINE_HEADERS})

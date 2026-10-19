add_library(QuasarEngine STATIC)
add_library(Quasar::Engine ALIAS QuasarEngine)

target_sources(QuasarEngine
    PRIVATE
        src/Version.cpp
        src/core/Log.cpp
        src/core/UUID.cpp
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
)

target_include_directories(QuasarEngine PRIVATE src)

target_link_libraries(QuasarEngine
    PUBLIC
        glm::glm
        imgui::imgui
        imguizmo::imguizmo
        spdlog::spdlog
        EnTT::EnTT
    PRIVATE
        Quasar::CompileOptions
        glfw
        glad::glad
        yaml-cpp::yaml-cpp
        assimp::assimp
        stb::image
        nfd::nfd
)

target_compile_definitions(QuasarEngine PRIVATE
    GLFW_INCLUDE_NONE
    QUASAR_VERSION="${PROJECT_VERSION}"
)

set_target_properties(QuasarEngine PROPERTIES FOLDER "Engine")

get_target_property(QUASAR_ENGINE_SOURCES QuasarEngine SOURCES)
get_target_property(QUASAR_ENGINE_HEADERS QuasarEngine HEADER_SET)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${QUASAR_ENGINE_SOURCES} ${QUASAR_ENGINE_HEADERS})
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(BUILD_SHARED_LIBS OFF)
set(CMAKE_DEBUG_POSTFIX "")

# doctest: unit test framework
set(DOCTEST_WITH_TESTS               OFF CACHE BOOL "" FORCE)
set(DOCTEST_WITH_MAIN_IN_STATIC_LIB  ON  CACHE BOOL "" FORCE)
set(DOCTEST_NO_INSTALL               ON  CACHE BOOL "" FORCE)
add_subdirectory(doctest EXCLUDE_FROM_ALL)

# GLFW: window and input library
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)
add_subdirectory(glfw EXCLUDE_FROM_ALL)

# GLM: math library, header only
set(GLM_BUILD_LIBRARY OFF CACHE BOOL "" FORCE)
set(GLM_BUILD_TESTS   OFF CACHE BOOL "" FORCE)
set(GLM_BUILD_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(glm EXCLUDE_FROM_ALL)

target_compile_definitions(glm INTERFACE
    GLM_FORCE_XYZW_ONLY
    GLM_FORCE_EXPLICIT_CTOR
    GLM_ENABLE_EXPERIMENTAL
)

# glad: OpenGL loader
add_subdirectory(glad EXCLUDE_FROM_ALL)

# imgui: immediate mode GUI library
add_subdirectory(imgui-cmake EXCLUDE_FROM_ALL)

# imguizmo: gizmo library for imgui
set(IMGUIZMO_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
add_subdirectory(imguizmo EXCLUDE_FROM_ALL)

target_link_libraries(imguizmo PUBLIC imgui::imgui)

if(MSVC)
    target_compile_options(imguizmo PRIVATE /W0)
else()
    target_compile_options(imguizmo PRIVATE -w)
endif()

# spdlog: logging library
set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS   OFF CACHE BOOL "" FORCE)
set(SPDLOG_INSTALL       OFF CACHE BOOL "" FORCE)
add_subdirectory(spdlog EXCLUDE_FROM_ALL)

# EnTT: entity-component-system library
set(ENTT_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(entt EXCLUDE_FROM_ALL)

# yaml-cpp: YAML parser and emitter
set(YAML_CPP_BUILD_TOOLS   OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "" FORCE)
set(YAML_CPP_INSTALL       OFF CACHE BOOL "" FORCE)
set(YAML_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(YAML_MSVC_SHARED_RT    ON  CACHE BOOL "" FORCE)
set_property(GLOBAL PROPERTY CTEST_TARGETS_ADDED 1)
add_subdirectory(yaml-cpp EXCLUDE_FROM_ALL)

if(MSVC)
    target_compile_options(yaml-cpp PRIVATE /wd4267)
endif()

# assimp: Open Asset Import Library
set(ASSIMP_BUILD_TESTS                    OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS             OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_SAMPLES                  OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL                        OFF CACHE BOOL "" FORCE)
set(ASSIMP_NO_EXPORT                      ON  CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_FBX_IMPORTER             ON  CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_GLTF_IMPORTER            ON  CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_OBJ_IMPORTER             ON  CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ZLIB                     ON  CACHE BOOL "" FORCE)
set(ASSIMP_WARNINGS_AS_ERRORS             OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_USE_CCACHE               OFF CACHE BOOL "" FORCE)
set(ASSIMP_INJECT_DEBUG_POSTFIX           OFF CACHE BOOL "" FORCE)
set(ASSIMP_IGNORE_GIT_HASH                ON  CACHE BOOL "" FORCE)
add_subdirectory(assimp EXCLUDE_FROM_ALL)

foreach(target IN ITEMS assimp zlibstatic)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W0)
    else()
        target_compile_options(${target} PRIVATE -w)
    endif()
endforeach()

# stb image: image loading library
add_subdirectory(stb-cmake EXCLUDE_FROM_ALL)

# nativefiledialog: cross-platform file dialog library
set(NFD_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(NFD_INSTALL     OFF CACHE BOOL "" FORCE)
set(NFD_PORTAL      OFF CACHE BOOL "" FORCE)
add_subdirectory(nfd EXCLUDE_FROM_ALL)

if(MSVC)
    target_compile_options(nfd PRIVATE /W0)
else()
    target_compile_options(nfd PRIVATE -w)
endif()

function(quasar_mark_external directory)
    get_property(targets DIRECTORY "${directory}" PROPERTY BUILDSYSTEM_TARGETS)
    foreach(target IN LISTS targets)
        set_property(TARGET ${target} PROPERTY FOLDER "External")
        get_target_property(target_type ${target} TYPE)
        if(NOT target_type STREQUAL "UTILITY")
            set_property(TARGET ${target} PROPERTY SYSTEM ON)
        endif()
    endforeach()

    get_property(subdirectories DIRECTORY "${directory}" PROPERTY SUBDIRECTORIES)
    foreach(subdirectory IN LISTS subdirectories)
        quasar_mark_external("${subdirectory}")
    endforeach()
endfunction()

quasar_mark_external("${CMAKE_CURRENT_SOURCE_DIR}")

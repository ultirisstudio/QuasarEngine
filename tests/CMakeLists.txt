file(GLOB_RECURSE QUASAR_TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h")
if(NOT QUASAR_OPENGL)
    list(FILTER QUASAR_TEST_SOURCES EXCLUDE REGEX "/src/platform/opengl/")
endif()
if(NOT QUASAR_VULKAN)
    list(FILTER QUASAR_TEST_SOURCES EXCLUDE REGEX "/src/platform/vulkan/")
endif()

add_executable(QuasarTests ${QUASAR_TEST_SOURCES})

target_include_directories(QuasarTests PRIVATE src)

target_link_libraries(QuasarTests PRIVATE
    Quasar::Engine
    Quasar::EditorLibrary
    Quasar::CompileOptions
    doctest::doctest_with_main
    SDL3::SDL3
    glad::glad
)

target_compile_definitions(QuasarTests PRIVATE
    QUASAR_TEST_EXPECTED_VERSION="${PROJECT_VERSION}"
)

set_target_properties(QuasarTests PROPERTIES FOLDER "Tests")
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" FILES ${QUASAR_TEST_SOURCES})

include("${PROJECT_SOURCE_DIR}/external/doctest/scripts/cmake/doctest.cmake")
# With the sanitizers, the Vulkan loader keeps the drivers and the layers loaded until the end, so that the leak
# sanitizer can name their modules and its suppressions match them
if(QUASAR_SANITIZERS)
    set(QUASAR_TEST_PROPERTIES PROPERTIES ENVIRONMENT VK_LOADER_DISABLE_DYNAMIC_LIBRARY_UNLOADING=1)
endif()
doctest_discover_tests(QuasarTests EXTRA_ARGS --no-intro=true ${QUASAR_TEST_PROPERTIES})

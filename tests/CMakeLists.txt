file(GLOB_RECURSE QUASAR_TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_executable(QuasarTests ${QUASAR_TEST_SOURCES})

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

include("${PROJECT_SOURCE_DIR}/external/doctest/scripts/cmake/doctest.cmake")
doctest_discover_tests(QuasarTests EXTRA_ARGS --no-intro=true)

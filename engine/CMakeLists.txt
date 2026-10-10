add_library(QuasarEngine STATIC)
add_library(Quasar::Engine ALIAS QuasarEngine)

target_sources(QuasarEngine
    PRIVATE
        src/Version.cpp
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS include
        FILES
            include/quasar/Version.h
)

target_include_directories(QuasarEngine PRIVATE src)

target_link_libraries(QuasarEngine PRIVATE Quasar::CompileOptions)

target_compile_definitions(QuasarEngine PRIVATE
    QUASAR_VERSION="${PROJECT_VERSION}"
)

set_target_properties(QuasarEngine PROPERTIES FOLDER "Engine")

get_target_property(QUASAR_ENGINE_SOURCES QuasarEngine SOURCES)
get_target_property(QUASAR_ENGINE_HEADERS QuasarEngine HEADER_SET)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${QUASAR_ENGINE_SOURCES} ${QUASAR_ENGINE_HEADERS})

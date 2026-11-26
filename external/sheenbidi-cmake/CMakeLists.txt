cmake_path(SET SHEENBIDI_DIR NORMALIZE "${CMAKE_CURRENT_SOURCE_DIR}/../SheenBidi")

add_library(SheenBidi STATIC ${SHEENBIDI_DIR}/Source/SheenBidi.c)
add_library(SheenBidi::SheenBidi ALIAS SheenBidi)

target_compile_definitions(SheenBidi PRIVATE SB_CONFIG_UNITY)
target_include_directories(SheenBidi
    PUBLIC ${SHEENBIDI_DIR}/Headers
    PRIVATE ${SHEENBIDI_DIR}/Source
)

if(MSVC)
    target_compile_options(SheenBidi PRIVATE /W0)
else()
    target_compile_options(SheenBidi PRIVATE -w)
endif()

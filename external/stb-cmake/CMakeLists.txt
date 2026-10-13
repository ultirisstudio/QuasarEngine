cmake_path(SET STB_DIR NORMALIZE "${CMAKE_CURRENT_SOURCE_DIR}/../stb")

add_library(stb_image STATIC stb_image.c)
add_library(stb::image ALIAS stb_image)

target_include_directories(stb_image PUBLIC ${STB_DIR})

if(MSVC)
    target_compile_options(stb_image PRIVATE /W0)
else()
    target_compile_options(stb_image PRIVATE -w)
endif()
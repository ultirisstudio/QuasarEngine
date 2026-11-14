cmake_path(SET IMGUI_DIR NORMALIZE "${CMAKE_CURRENT_SOURCE_DIR}/../imgui")

add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
)
add_library(imgui::imgui ALIAS imgui)

target_include_directories(imgui PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

target_link_libraries(imgui PRIVATE SDL3::SDL3 ${CMAKE_DL_LIBS})

if(QUASAR_OPENGL)
    target_sources(imgui PRIVATE ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp)
endif()

if(QUASAR_VULKAN)
    target_sources(imgui PRIVATE ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp)
    target_link_libraries(imgui PRIVATE Vulkan::Vulkan)
endif()

if(MSVC)
    target_compile_options(imgui PRIVATE /W0)
else()
    target_compile_options(imgui PRIVATE -w)
endif()
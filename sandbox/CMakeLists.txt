add_executable(QuasarSandbox)

target_sources(QuasarSandbox PRIVATE
    src/main.cpp
)

target_link_libraries(QuasarSandbox PRIVATE
    Quasar::Engine
    Quasar::CompileOptions
)

set_target_properties(QuasarSandbox PROPERTIES FOLDER "Applications")

add_executable(QuasarEditor)

target_sources(QuasarEditor PRIVATE
    src/main.cpp
)

target_link_libraries(QuasarEditor PRIVATE
    Quasar::Engine
    Quasar::CompileOptions
)

set_target_properties(QuasarEditor PROPERTIES FOLDER "Applications")

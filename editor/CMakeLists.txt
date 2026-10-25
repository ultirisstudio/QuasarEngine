add_executable(QuasarEditor)

target_sources(QuasarEditor PRIVATE
    src/EditorApplication.cpp
)

target_link_libraries(QuasarEditor PRIVATE
    Quasar::EntryPoint
    Quasar::CompileOptions
)

set_target_properties(QuasarEditor PROPERTIES FOLDER "Applications")

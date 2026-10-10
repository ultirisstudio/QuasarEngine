# doctest: unit test framework
set(DOCTEST_WITH_TESTS               OFF CACHE BOOL "" FORCE)
set(DOCTEST_WITH_MAIN_IN_STATIC_LIB  ON  CACHE BOOL "" FORCE)
set(DOCTEST_NO_INSTALL               ON  CACHE BOOL "" FORCE)
add_subdirectory(doctest EXCLUDE_FROM_ALL)

function(quasar_mark_external directory)
    get_property(targets DIRECTORY "${directory}" PROPERTY BUILDSYSTEM_TARGETS)
    foreach(target IN LISTS targets)
        set_property(TARGET ${target} PROPERTY FOLDER "External")
        get_target_property(target_type ${target} TYPE)
        if(NOT target_type STREQUAL "UTILITY" AND NOT CMAKE_VERSION VERSION_LESS 3.25)
            set_property(TARGET ${target} PROPERTY SYSTEM ON)
        endif()
    endforeach()

    get_property(subdirectories DIRECTORY "${directory}" PROPERTY SUBDIRECTORIES)
    foreach(subdirectory IN LISTS subdirectories)
        quasar_mark_external("${subdirectory}")
    endforeach()
endfunction()

quasar_mark_external("${CMAKE_CURRENT_SOURCE_DIR}")

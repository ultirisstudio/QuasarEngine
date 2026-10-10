# Compiler options shared by every QuasarEngine target.
#
# Usage: target_link_libraries(<target> PRIVATE Quasar::CompileOptions)

option(QUASAR_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)

add_library(quasar_compile_options INTERFACE)
add_library(Quasar::CompileOptions ALIAS quasar_compile_options)

target_compile_definitions(quasar_compile_options INTERFACE
    $<$<CONFIG:Debug>:QUASAR_DEBUG>
    $<$<CONFIG:Release>:QUASAR_RELEASE>
)

if(MSVC)
    target_compile_options(quasar_compile_options INTERFACE
        # Standard conformance
        /permissive-
        /utf-8
        /Zc:__cplusplus
        /Zc:preprocessor
        /Zc:inline
        /Zc:throwingNew

        # Warnings, with headers from third-party code excluded
        /W4
        /external:W0
        /w14062   # enumerator not handled in switch
        /w14242   # conversion, possible loss of data
        /w14254   # larger bit field type converted to smaller one
        /w14263   # member function does not override a base class virtual function
        /w14265   # class has virtual functions but no virtual destructor
        /w14287   # unsigned and negative constant mismatch
        /w14289   # loop variable used outside the for loop
        /w14296   # expression is always true or always false
        /w14311   # pointer truncation
        /w14545   # expression before comma evaluates to a function
        /w14546   # function call before comma missing argument list
        /w14547   # operator before comma has no effect
        /w14549   # operator before comma has no effect
        /w14555   # expression has no effect
        /w14619   # unknown warning number in a pragma
        /w14640   # thread-unsafe static member initialization
        /w14826   # sign-extended conversion
        /w14905   # wide string literal cast to LPSTR
        /w14906   # string literal cast to LPWSTR
        /w14928   # illegal copy-initialization
        /w15038   # member initialization order
        /w15204   # class with virtual functions has a non-virtual destructor
        /w15219   # implicit conversion, possible loss of data
        /w15240   # attribute ignored in this position
        /w15262   # implicit fall-through
        /w15263   # std::move on a temporary prevents copy elision

        # Faster builds with the Visual Studio generator
        /MP

        # Extra runtime checks in Debug
        $<$<CONFIG:Debug>:/sdl>
    )

    if(QUASAR_WARNINGS_AS_ERRORS)
        target_compile_options(quasar_compile_options INTERFACE /WX)
    endif()
else()
    target_compile_options(quasar_compile_options INTERFACE
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
        -Wconversion
        -Wsign-conversion
        -Wdouble-promotion
        -Wcast-align
        -Wcast-qual
        -Wnull-dereference
        -Wimplicit-fallthrough
        -Wmisleading-indentation
        -Wformat=2

        # C++ only
        $<$<COMPILE_LANGUAGE:CXX>:-Wold-style-cast>
        $<$<COMPILE_LANGUAGE:CXX>:-Wnon-virtual-dtor>
        $<$<COMPILE_LANGUAGE:CXX>:-Woverloaded-virtual>
        $<$<COMPILE_LANGUAGE:CXX>:-Wextra-semi>
    )

    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(quasar_compile_options INTERFACE
            -Wduplicated-cond
            -Wduplicated-branches
            -Wlogical-op
            $<$<COMPILE_LANGUAGE:CXX>:-Wuseless-cast>
        )
    endif()

    # Bounds checks in the standard library containers, Debug only
    target_compile_definitions(quasar_compile_options INTERFACE
        $<$<CONFIG:Debug>:_GLIBCXX_ASSERTIONS>
    )

    if(QUASAR_WARNINGS_AS_ERRORS)
        target_compile_options(quasar_compile_options INTERFACE -Werror)
    endif()
endif()

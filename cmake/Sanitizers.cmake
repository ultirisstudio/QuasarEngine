# Usage: cmake --preset linux-clang-asan, or set QUASAR_SANITIZERS when configuring, for example
# -DQUASAR_SANITIZERS="address;undefined"
#
# - address: out of bounds accesses, use after free, double free and, on Linux, memory leaks
# - undefined: undefined behavior, such as signed overflow, invalid casts or null pointer accesses (GCC and Clang)
# - thread: data races between threads (GCC and Clang, cannot be used with address)

set(QUASAR_SANITIZERS "" CACHE STRING "Sanitizers to enable, separated by semicolons: address, undefined, thread")

if(NOT QUASAR_SANITIZERS)
    return()
endif()

foreach(sanitizer IN LISTS QUASAR_SANITIZERS)
    if(NOT sanitizer MATCHES "^(address|undefined|thread)$")
        message(FATAL_ERROR "Unknown sanitizer '${sanitizer}' in QUASAR_SANITIZERS. Use address, undefined or thread.")
    endif()
endforeach()

if("address" IN_LIST QUASAR_SANITIZERS AND "thread" IN_LIST QUASAR_SANITIZERS)
    message(FATAL_ERROR "The address and thread sanitizers cannot be used together.")
endif()

if(MSVC)
    if(NOT QUASAR_SANITIZERS STREQUAL "address")
        message(FATAL_ERROR "MSVC only supports the address sanitizer.")
    endif()

    add_compile_options(/fsanitize=address)
    add_link_options(/INCREMENTAL:NO)

    foreach(language IN ITEMS C CXX)
        string(REGEX REPLACE "/RTC[1csu]*" "" CMAKE_${language}_FLAGS_DEBUG "${CMAKE_${language}_FLAGS_DEBUG}")
    endforeach()

    get_filename_component(QUASAR_COMPILER_DIRECTORY "${CMAKE_CXX_COMPILER}" DIRECTORY)
    file(GLOB QUASAR_SANITIZER_RUNTIMES "${QUASAR_COMPILER_DIRECTORY}/clang_rt.asan*_dynamic-x86_64.dll")
    if(NOT QUASAR_SANITIZER_RUNTIMES)
        message(FATAL_ERROR "The runtime library of the address sanitizer is not in ${QUASAR_COMPILER_DIRECTORY}.")
    endif()

    foreach(configuration IN LISTS CMAKE_CONFIGURATION_TYPES)
        file(COPY ${QUASAR_SANITIZER_RUNTIMES} DESTINATION "${CMAKE_BINARY_DIR}/bin/${configuration}")
    endforeach()
else()
    set(QUASAR_SANITIZE_FLAGS ${QUASAR_SANITIZERS})

    if("undefined" IN_LIST QUASAR_SANITIZERS AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        list(APPEND QUASAR_SANITIZE_FLAGS float-cast-overflow)
    endif()

    list(JOIN QUASAR_SANITIZE_FLAGS "," QUASAR_SANITIZE_FLAGS)

    add_compile_options(
        -fsanitize=${QUASAR_SANITIZE_FLAGS}
        -fno-sanitize-recover=all
        -fno-omit-frame-pointer
        -fno-optimize-sibling-calls
    )
    add_link_options(-fsanitize=${QUASAR_SANITIZE_FLAGS})

    if("thread" IN_LIST QUASAR_SANITIZERS)
        add_compile_options(-fno-sanitize=vptr)
        add_link_options(-fno-sanitize=vptr)
    endif()

    if("address" IN_LIST QUASAR_SANITIZERS)
        add_compile_definitions(_GLIBCXX_SANITIZE_VECTOR)
    endif()

    target_sources(quasar_compile_options INTERFACE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CMAKE_CURRENT_LIST_DIR}/SanitizerOptions.cpp>
    )
endif()
function(qe_configure_target target folder_name)
  if(NOT TARGET "${target}")
    message(FATAL_ERROR "qe_configure_target: target '${target}' doesn't exist")
  endif()

  if(WIN32)
    set(qe_system "Windows")
  else()
    set(qe_system "${CMAKE_SYSTEM_NAME}")
  endif()

  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(qe_arch "x86_64")
  else()
    set(qe_arch "x86")
  endif()

  set(qe_subdir "$<CONFIG>-${qe_system}-${qe_arch}")

  set_target_properties("${target}" PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${QE_BIN_DIR}/${qe_subdir}/${target}"
    LIBRARY_OUTPUT_DIRECTORY "${QE_BIN_DIR}/${qe_subdir}/${target}"
    ARCHIVE_OUTPUT_DIRECTORY "${QE_BIN_DIR}/${qe_subdir}/${target}"
  )

  set_target_properties("${target}" PROPERTIES
    FOLDER "${folder_name}"
  )

  target_compile_definitions("${target}" PRIVATE
    $<$<CONFIG:Debug>:DEBUG;_DEBUG>
    $<$<CONFIG:Release>:RELEASE;NDEBUG>
  )
endfunction()

function(qe_enable_bigobj target)
  if(MSVC)
    target_compile_options("${target}" PRIVATE /bigobj)
  endif()
endfunction()

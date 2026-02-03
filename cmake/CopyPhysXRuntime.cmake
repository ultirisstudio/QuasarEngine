if(NOT DEFINED src OR NOT DEFINED dst)
  message(FATAL_ERROR "CopyPhysXRuntime.cmake: -Dsrc=... -Ddst=... required")
endif()

file(MAKE_DIRECTORY "${dst}")

set(dlls
  PhysXFoundation_64.dll
  PhysXCommon_64.dll
  PhysX_64.dll
  PhysXCooking_64.dll
  PVDRuntime_64.dll
)

foreach(d IN LISTS dlls)
  if(EXISTS "${src}/${d}")
    file(COPY_FILE "${src}/${d}" "${dst}/${d}" ONLY_IF_DIFFERENT)
  endif()
endforeach()

if(NOT DEFINED src OR NOT DEFINED dst)
  message(FATAL_ERROR "CopyPhysXRuntime.cmake: 'src' and 'dst' must be defined")
endif()

string(REPLACE "\"" "" src "${src}")
string(REPLACE "\"" "" dst "${dst}")

get_filename_component(src "${src}" ABSOLUTE)
get_filename_component(dst "${dst}" ABSOLUTE)

file(MAKE_DIRECTORY "${dst}")

file(GLOB _dlls "${src}/*.dll")
if(_dlls)
  file(COPY ${_dlls} DESTINATION "${dst}")
endif()

file(GLOB _pdbs "${src}/*.pdb")
if(_pdbs)
  file(COPY ${_pdbs} DESTINATION "${dst}")
endif()

message(STATUS "PhysX runtime copied from '${src}' to '${dst}'")

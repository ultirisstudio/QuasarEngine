set(VULKAN_SDK "$ENV{VULKAN_SDK}")

set(QE_VENDOR_DIR "${CMAKE_SOURCE_DIR}/vendor")

set(QE_INC_GLFW            "${QE_VENDOR_DIR}/GLFW/include")
set(QE_INC_Glad            "${QE_VENDOR_DIR}/Glad/include")
set(QE_INC_glm             "${QE_VENDOR_DIR}/glm")
set(QE_INC_ImGui           "${QE_VENDOR_DIR}/ImGui")
set(QE_INC_stb 			   "${QE_VENDOR_DIR}/stb/include")
set(QE_INC_yaml_cpp        "${QE_VENDOR_DIR}/yaml_cpp/include")
set(QE_INC_ImGuizmo        "${QE_VENDOR_DIR}/ImGuizmo")
set(QE_INC_tinyfiledialogs "${QE_VENDOR_DIR}/tinyfiledialogs/include")
set(QE_INC_mbedtls         "${QE_VENDOR_DIR}/mbedtls/include")
set(QE_INC_zlib            "${QE_VENDOR_DIR}/zlib/include")
set(QE_INC_entt            "${QE_VENDOR_DIR}/entt")
set(QE_INC_TextEditor      "${QE_VENDOR_DIR}/TextEditor/include")
set(QE_INC_PhysX           "${QE_VENDOR_DIR}/PhysX/physx/include")
set(QE_INC_assimp_fallback "${QE_VENDOR_DIR}/assimp/include")

if(VULKAN_SDK)
  set(QE_INC_VulkanSDK "${VULKAN_SDK}/Include")
  set(QE_LIB_VulkanSDK "${VULKAN_SDK}/Lib")
endif()

function(qe_ensure_iface_target name)
  if(NOT TARGET "${name}")
    add_library("${name}" INTERFACE)
  endif()
endfunction()

if(NOT COMMAND qe_set_folder_recursive)
  function(qe_set_folder_recursive dir folder)
    get_property(_targets DIRECTORY "${dir}" PROPERTY BUILDSYSTEM_TARGETS)
    foreach(t IN LISTS _targets)
      if(TARGET "${t}")
        set_target_properties("${t}" PROPERTIES FOLDER "${folder}")
      endif()
    endforeach()

    get_property(_subdirs DIRECTORY "${dir}" PROPERTY SUBDIRECTORIES)
    foreach(sd IN LISTS _subdirs)
      qe_set_folder_recursive("${sd}" "${folder}")
    endforeach()
  endfunction()
endif()

# -----------------------------------------
# Header-only
# -----------------------------------------
qe_ensure_iface_target(glm)
target_include_directories(glm INTERFACE "${QE_INC_glm}")

qe_ensure_iface_target(entt)
target_include_directories(entt INTERFACE "${QE_INC_entt}")

qe_ensure_iface_target(PhysXHeaders)
target_include_directories(PhysXHeaders INTERFACE "${QE_INC_PhysX}")

foreach(lib zlib)
  qe_ensure_iface_target(${lib})
endforeach()

target_include_directories(zlib            INTERFACE "${QE_INC_zlib}")

# -----------------------------------------
# Vulkan
# -----------------------------------------
find_package(Vulkan QUIET)

if(NOT Vulkan_FOUND)
  if(WIN32 AND VULKAN_SDK AND EXISTS "${QE_LIB_VulkanSDK}/vulkan-1.lib")
    if(NOT TARGET Vulkan::Vulkan)
      add_library(Vulkan::Vulkan UNKNOWN IMPORTED)
      set_target_properties(Vulkan::Vulkan PROPERTIES
        IMPORTED_LOCATION "${QE_LIB_VulkanSDK}/vulkan-1.lib"
        INTERFACE_INCLUDE_DIRECTORIES "${QE_INC_VulkanSDK}"
      )
    endif()
  endif()
endif()

# -----------------------------------------
# PhysX
# -----------------------------------------
option(QE_BUILD_PHYSX "Build PhysX from vendor source" ON)
set(QE_PHYSX_PRESET "vc17win64-cpu-only" CACHE STRING "PhysX preset name (generate_projects)")

if(QE_BUILD_PHYSX)
  set(PHYSX_ROOT "${QE_VENDOR_DIR}/PhysX/physx")

  if(WIN32)
    set(PHYSX_GEN_SCRIPT "${PHYSX_ROOT}/generate_projects.bat")
  else()
    set(PHYSX_GEN_SCRIPT "${PHYSX_ROOT}/generate_projects.sh")
  endif()

  set(PHYSX_COMPILER_DIR "${PHYSX_ROOT}/compiler/${QE_PHYSX_PRESET}")

  set(PHYSX_GEN_STAMP "${CMAKE_BINARY_DIR}/physx_${QE_PHYSX_PRESET}_generated.stamp")
  add_custom_command(
    OUTPUT "${PHYSX_GEN_STAMP}"
    COMMAND "${PHYSX_GEN_SCRIPT}" "${QE_PHYSX_PRESET}"
    COMMAND "${CMAKE_COMMAND}" -E touch "${PHYSX_GEN_STAMP}"
    WORKING_DIRECTORY "${PHYSX_ROOT}"
    VERBATIM
  )
  add_custom_target(PhysX_Generate DEPENDS "${PHYSX_GEN_STAMP}")
  set_target_properties(PhysX_Generate PROPERTIES FOLDER "Dependencies/PhysX")

  set(PHYSX_BUILD_STAMP_CHECKED "${CMAKE_BINARY_DIR}/physx_${QE_PHYSX_PRESET}_checked.stamp")
  set(PHYSX_BUILD_STAMP_RELEASE "${CMAKE_BINARY_DIR}/physx_${QE_PHYSX_PRESET}_release.stamp")

  add_custom_command(
    OUTPUT "${PHYSX_BUILD_STAMP_CHECKED}"
    COMMAND "${CMAKE_COMMAND}" --build "${PHYSX_COMPILER_DIR}" --config checked
    COMMAND "${CMAKE_COMMAND}" -E touch "${PHYSX_BUILD_STAMP_CHECKED}"
    DEPENDS PhysX_Generate
    VERBATIM
  )

  add_custom_command(
    OUTPUT "${PHYSX_BUILD_STAMP_RELEASE}"
    COMMAND "${CMAKE_COMMAND}" --build "${PHYSX_COMPILER_DIR}" --config release
    COMMAND "${CMAKE_COMMAND}" -E touch "${PHYSX_BUILD_STAMP_RELEASE}"
    DEPENDS PhysX_Generate
    VERBATIM
  )

  add_custom_target(PhysX_Build ALL
    DEPENDS "${PHYSX_BUILD_STAMP_CHECKED}" "${PHYSX_BUILD_STAMP_RELEASE}"
  )
  set_target_properties(PhysX_Build PROPERTIES FOLDER "Dependencies/PhysX")

  if(WIN32)
    set(PHYSX_RT "mt" CACHE STRING "PhysX runtime tag folder (mt/md)")
    set(QE_PHYSX_BIN_VC "")

    if(QE_PHYSX_PRESET MATCHES "^vc17")
      set(QE_PHYSX_BIN_VC "vc143")
    elseif(QE_PHYSX_PRESET MATCHES "^vc16")
      set(QE_PHYSX_BIN_VC "vc142")
    else()
      set(QE_PHYSX_BIN_VC "vc143")
    endif()

    set(QE_PHYSX_BIN_VC "${QE_PHYSX_BIN_VC}" CACHE STRING "PhysX bin VC tag folder (vc142/vc143/...)")

    set(PHYSX_BIN_BASE    "${PHYSX_ROOT}/bin/win.x86_64.${QE_PHYSX_BIN_VC}.${PHYSX_RT}")
    set(PHYSX_BIN_CHECKED "${PHYSX_BIN_BASE}/checked")
    set(PHYSX_BIN_RELEASE "${PHYSX_BIN_BASE}/release")
  endif()

  function(qe_physx_import libname)
    if(NOT WIN32)
      return()
    endif()

    add_library(${libname} UNKNOWN IMPORTED GLOBAL)
    set_target_properties(${libname} PROPERTIES
      IMPORTED_CONFIGURATIONS "Debug;Release;RelWithDebInfo;MinSizeRel"

      IMPORTED_LOCATION_DEBUG          "${PHYSX_BIN_CHECKED}/${libname}.lib"
      IMPORTED_LOCATION_RELEASE        "${PHYSX_BIN_RELEASE}/${libname}.lib"
      IMPORTED_LOCATION_RELWITHDEBINFO "${PHYSX_BIN_RELEASE}/${libname}.lib"
      IMPORTED_LOCATION_MINSIZEREL     "${PHYSX_BIN_RELEASE}/${libname}.lib"
    )
  endfunction()

  foreach(LIB
    PhysXExtensions_static_64
    PhysXCooking_64
    PhysXCharacterKinematic_static_64
    PhysXVehicle2_static_64
    PhysX_64
    SimulationController_static_64
    SceneQuery_static_64
    PhysXTask_static_64
    LowLevelDynamics_static_64
    LowLevelAABB_static_64
    LowLevel_static_64
    PhysXCommon_64
    PhysXFoundation_64
    PhysXPvdSDK_static_64
    PVDRuntime_64
  )
    qe_physx_import(${LIB})
  endforeach()

  add_library(PhysXSDK INTERFACE)
  set_target_properties(PhysXSDK PROPERTIES FOLDER "Dependencies/PhysX")
  target_include_directories(PhysXSDK INTERFACE "${PHYSX_ROOT}/include")

  if(WIN32)
    target_link_libraries(PhysXSDK INTERFACE
      PhysXExtensions_static_64
      PhysXCooking_64
      PhysXCharacterKinematic_static_64
      PhysXVehicle2_static_64
      PhysX_64
      SimulationController_static_64
      SceneQuery_static_64
      PhysXTask_static_64
      LowLevelDynamics_static_64
      LowLevelAABB_static_64
      LowLevel_static_64
      PhysXCommon_64
      PhysXFoundation_64
      PhysXPvdSDK_static_64
      PVDRuntime_64
    )
  endif()
endif()

# -----------------------------------------
# Assimp
# -----------------------------------------
option(QE_BUILD_ASSIMP "Build assimp from vendor source" ON)

if(QE_BUILD_ASSIMP)
  set(_QE_OLD_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})

  set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libs" FORCE)

  set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
  set(ASSIMP_BUILD_TESTS        OFF CACHE BOOL "" FORCE)
  set(ASSIMP_BUILD_SAMPLES      OFF CACHE BOOL "" FORCE)
  set(ASSIMP_INSTALL            OFF CACHE BOOL "" FORCE)

  set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "" FORCE)

  add_subdirectory(
    "${QE_VENDOR_DIR}/assimp"
    "${CMAKE_BINARY_DIR}/vendor/assimp"
  )

  set(BUILD_SHARED_LIBS ${_QE_OLD_BUILD_SHARED_LIBS} CACHE BOOL "Build shared libs" FORCE)

  if(TARGET assimp::assimp)
  elseif(TARGET assimp)
    add_library(assimp::assimp ALIAS assimp)
  elseif(TARGET assimp_static)
    add_library(assimp::assimp ALIAS assimp_static)
  endif()

  qe_set_folder_recursive("${QE_VENDOR_DIR}/assimp" "Dependencies/assimp")
else()
  qe_ensure_iface_target(assimp)
  target_include_directories(assimp INTERFACE "${QE_INC_assimp_fallback}")
endif()

# -------------------------
# GLFW
# -------------------------
option(QE_BUILD_GLFW "Build GLFW from vendor source" ON)

if(QE_BUILD_GLFW)
  set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
  set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)

  set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libs" FORCE)

  add_subdirectory(
    "${CMAKE_SOURCE_DIR}/vendor/GLFW"
    "${CMAKE_BINARY_DIR}/vendor/GLFW"
    EXCLUDE_FROM_ALL
  )

  if(TARGET glfw AND NOT TARGET GLFW)
    add_library(GLFW ALIAS glfw)
  endif()

  if(TARGET glfw)
    set_target_properties(glfw PROPERTIES FOLDER "Dependencies/GLFW")
  endif()
else()
  qe_ensure_iface_target(GLFW)
  target_include_directories(GLFW INTERFACE "${QE_INC_GLFW}")
endif()

# -------------------------
# yaml-cpp
# -------------------------
option(QE_BUILD_YAML_CPP "Build yaml-cpp from vendor source" ON)

if(QE_BUILD_YAML_CPP)
  set(YAML_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

  set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
  set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)

  add_subdirectory(
    "${CMAKE_SOURCE_DIR}/vendor/yaml_cpp"
    "${CMAKE_BINARY_DIR}/vendor/yaml_cpp"
    EXCLUDE_FROM_ALL
  )

  if(TARGET yaml-cpp::yaml-cpp AND NOT TARGET yaml-cpp)
    add_library(yaml-cpp ALIAS yaml-cpp::yaml-cpp)
  endif()

  if(TARGET yaml-cpp)
    set_target_properties(yaml-cpp PROPERTIES FOLDER "Dependencies/yaml-cpp")
  endif()

else()
  qe_ensure_iface_target(yaml-cpp)
  target_include_directories(yaml-cpp INTERFACE "${QE_INC_yaml_cpp}")
endif()

# -------------------------
# ImGui
# -------------------------
option(QE_BUILD_IMGUI "Build ImGui from vendor source" ON)

if(QE_BUILD_IMGUI)
	set(QE_IMGUI_WITH_GLFW   ON  CACHE BOOL "" FORCE)
	set(QE_IMGUI_WITH_VULKAN ON  CACHE BOOL "" FORCE)
	set(QE_IMGUI_WITH_DX11   ON  CACHE BOOL "" FORCE)
	set(QE_IMGUI_WITH_OPENGL3 ON CACHE BOOL "" FORCE)
  add_subdirectory(
    "${CMAKE_SOURCE_DIR}/vendor/ImGui"
    "${CMAKE_BINARY_DIR}/vendor/ImGui"
    EXCLUDE_FROM_ALL
  )
else()
  qe_ensure_iface_target(ImGui)
  target_include_directories(ImGui INTERFACE "${QE_INC_ImGui}")
endif()

# -------------------------
# Glad
# -------------------------
option(QE_BUILD_GLAD "Build Glad from vendor source" ON)

if(QE_BUILD_GLAD)
  add_subdirectory(
    "${CMAKE_SOURCE_DIR}/vendor/Glad"
    "${CMAKE_BINARY_DIR}/vendor/Glad"
    EXCLUDE_FROM_ALL
  )
else()
  qe_ensure_iface_target(Glad)
  target_include_directories(Glad INTERFACE "${QE_INC_Glad}")
endif()

# -------------------------
# ImGuizmo
# -------------------------
option(QE_BUILD_IMGUIZMO "Build ImGuizmo from vendor source" ON)

if(QE_BUILD_IMGUIZMO)
  add_subdirectory(
    "${CMAKE_SOURCE_DIR}/vendor/ImGuizmo"
    "${CMAKE_BINARY_DIR}/vendor/ImGuizmo"
    EXCLUDE_FROM_ALL
  )
else()
  qe_ensure_iface_target(ImGuizmo)
  target_include_directories(ImGuizmo INTERFACE "${QE_INC_ImGuizmo}")
endif()

# -------------------------
# stb
# -------------------------
option(QE_BUILD_STB "Build stb from vendor source" ON)

if(QE_BUILD_STB)
  add_subdirectory(
    "${CMAKE_SOURCE_DIR}/vendor/stb"
    "${CMAKE_BINARY_DIR}/vendor/stb"
    EXCLUDE_FROM_ALL
  )
else()
  qe_ensure_iface_target(stb)
  target_include_directories(stb INTERFACE "${CMAKE_SOURCE_DIR}/vendor/stb/include")
endif()

# -------------------------
# tinyfiledialogs
# -------------------------
option(QE_BUILD_TINYFILEDIALOGS "Build tinyfiledialogs from vendor source" ON)

if(QE_BUILD_TINYFILEDIALOGS)
  add_subdirectory(
    "${CMAKE_SOURCE_DIR}/vendor/tinyfiledialogs"
    "${CMAKE_BINARY_DIR}/vendor/tinyfiledialogs"
    EXCLUDE_FROM_ALL
  )
else()
  qe_ensure_iface_target(tinyfiledialogs)
  target_include_directories(tinyfiledialogs INTERFACE "${CMAKE_SOURCE_DIR}/vendor/tinyfiledialogs/include")
endif()

# -------------------------
# TextEditor
# -------------------------
option(QE_BUILD_TEXTEDITOR "Build TextEditor from vendor source" ON)

if(QE_BUILD_TEXTEDITOR)
  add_subdirectory(
    "${CMAKE_SOURCE_DIR}/vendor/TextEditor"
    "${CMAKE_BINARY_DIR}/vendor/TextEditor"
    EXCLUDE_FROM_ALL
  )
else()
  qe_ensure_iface_target(TextEditor)
  target_include_directories(TextEditor INTERFACE "${CMAKE_SOURCE_DIR}/vendor/TextEditor/include")
endif()

add_subdirectory("${CMAKE_SOURCE_DIR}/vendor/lua" "${CMAKE_BINARY_DIR}/vendor/lua")

add_subdirectory("${CMAKE_SOURCE_DIR}/vendor/sol2" "${CMAKE_BINARY_DIR}/vendor/sol2")

add_library(Lua::Lua ALIAS lua)
add_library(sol2::sol2 ALIAS sol2)



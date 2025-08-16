PHYSX_ROOT = "physx"
OUTPUT_DIR = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

USE_PVD        = false
USE_VEHICLE    = true
USE_CHARACTER  = true

local function physx_includedirs()
    includedirs {
        PHYSX_ROOT .. "/include",

        PHYSX_ROOT .. "/source/common/include",
        PHYSX_ROOT .. "/source/foundation/include",
        PHYSX_ROOT .. "/source/geomutils/include",
        PHYSX_ROOT .. "/source/physx/include",
        PHYSX_ROOT .. "/source/physxcooking/include",
        PHYSX_ROOT .. "/source/extensions/include",

        PHYSX_ROOT .. "/source/lowlevel/api/include",
        PHYSX_ROOT .. "/source/lowlevel/common/include",
        PHYSX_ROOT .. "/source/lowlevel/software/include",
        PHYSX_ROOT .. "/source/lowlevelaabb/include",
        PHYSX_ROOT .. "/source/lowleveldynamics/include",
        PHYSX_ROOT .. "/source/lowlevel/solver/include",

        PHYSX_ROOT .. "/source/characterkinematic/include",
        PHYSX_ROOT .. "/source/vehicle/include",
        PHYSX_ROOT .. "/source/pvd/include"
    }
end

local function physx_common_defines()
    defines {
        "PX_PHYSX_STATIC_LIBS",
        "PX_DISABLE_FLUIDS",
        "PX_DISABLE_CLOTH",
        USE_PVD and "PX_SUPPORT_PVD=1" or "PX_SUPPORT_PVD=0"
    }
end

local function physx_platform()
    filter "system:windows"
        systemversion "latest"
        defines { "_CRT_SECURE_NO_WARNINGS", "WIN32", "_WINDOWS" }
    filter {}
end

local function physx_cfg()
    filter "configurations:Debug"
        defines { "_DEBUG" }
        runtime "Debug"
        symbols "On"
    filter "configurations:Release"
        defines { "NDEBUG" }
        runtime "Release"
        optimize "On"
    filter {}
end

local function new_physx_lib(name)
    project(name)
        kind "StaticLib"
        language "C++"
        cppdialect "C++17"
        staticruntime "on"

        targetdir ("bin/" .. OUTPUT_DIR .. "/%{prj.name}")
        objdir    ("bin-int/" .. OUTPUT_DIR .. "/%{prj.name}")

        physx_includedirs()
        physx_common_defines()
        physx_platform()
        physx_cfg()
end

group "PhysX"

new_physx_lib("PhysXFoundation")
files {
    PHYSX_ROOT .. "/source/foundation/src/**.cpp",
    PHYSX_ROOT .. "/source/foundation/include/**.h"
}

new_physx_lib("PhysXCommon")
files {
    PHYSX_ROOT .. "/source/common/src/**.cpp",
    PHYSX_ROOT .. "/source/common/include/**.h"
}
links { "PhysXFoundation" }

new_physx_lib("PhysX")
files {
    PHYSX_ROOT .. "/source/physx/src/**.cpp",
    PHYSX_ROOT .. "/source/physx/include/**.h",

    PHYSX_ROOT .. "/source/geomutils/src/**.cpp",
    PHYSX_ROOT .. "/source/geomutils/include/**.h",

    PHYSX_ROOT .. "/source/lowlevel/**/src/**.cpp",
    PHYSX_ROOT .. "/source/lowlevel/**/include/**.h",
    PHYSX_ROOT .. "/source/lowlevelaabb/**/src/**.cpp",
    PHYSX_ROOT .. "/source/lowlevelaabb/**/include/**.h",
    PHYSX_ROOT .. "/source/lowleveldynamics/**/src/**.cpp",
    PHYSX_ROOT .. "/source/lowleveldynamics/**/include/**.h",

    PHYSX_ROOT .. "/source/physxcharacterkinematic/src/gjk/**.cpp",
}
links { "PhysXCommon", "PhysXFoundation" }

new_physx_lib("PhysXCooking")
files {
    PHYSX_ROOT .. "/source/physxcooking/src/**.cpp",
    PHYSX_ROOT .. "/source/physxcooking/include/**.h"
}
links { "PhysX", "PhysXCommon", "PhysXFoundation" }

new_physx_lib("PhysXExtensions")
files {
    PHYSX_ROOT .. "/source/extensions/src/**.cpp",
    PHYSX_ROOT .. "/source/extensions/include/**.h"
}
links { "PhysX", "PhysXCommon", "PhysXFoundation" }

if USE_CHARACTER then
    new_physx_lib("PhysXCharacterKinematic")
    files {
        PHYSX_ROOT .. "/source/characterkinematic/src/**.cpp",
        PHYSX_ROOT .. "/source/characterkinematic/include/**.h"
    }
    links { "PhysX", "PhysXCommon", "PhysXFoundation" }
    defines { "PX_ENABLE_CHARACTER=1" }
else
    defines { "PX_ENABLE_CHARACTER=0" }
end

if USE_VEHICLE then
    new_physx_lib("PhysXVehicle")
    files {
        PHYSX_ROOT .. "/source/vehicle/src/**.cpp",
        PHYSX_ROOT .. "/source/vehicle/include/**.h"
    }
    links { "PhysX", "PhysXCommon", "PhysXFoundation" }
    defines { "PX_ENABLE_VEHICLE=1" }
else
    defines { "PX_ENABLE_VEHICLE=0" }
end

group "Meta"

project "PhysX-All"
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"
    staticruntime "on"

    targetdir ("bin/" .. OUTPUT_DIR .. "/%{prj.name}")
    objdir    ("bin-int/" .. OUTPUT_DIR .. "/%{prj.name}")

    physx_includedirs()
    physx_common_defines()
    physx_platform()
    physx_cfg()

    links {
        "PhysXExtensions",
        "PhysXCooking",
        USE_CHARACTER and "PhysXCharacterKinematic" or nil,
        USE_VEHICLE   and "PhysXVehicle" or nil,
        "PhysX",
        "PhysXCommon",
        "PhysXFoundation"
    }

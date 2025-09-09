-- Root + output ---------------------------------------------------------------
PHYSX_ROOT  = "physx"
OUTPUT_DIR  = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

-- Feature toggles -------------------------------------------------------------
PUBLIC_RELEASE      = true
USE_CUDA_DELAYLOAD  = false
USE_PVD            = false
USE_VEHICLE        = true
USE_CHARACTER      = true

-- Common helpers --------------------------------------------------------------
local function physx_common_defines()
    defines {
        "PX_PHYSX_STATIC_LIB",
        "PX_DISABLE_FLUIDS",
        "PX_DISABLE_CLOTH",
        "PX_SUPPORT_GPU_PHYSX=0",
        USE_PVD       and "PX_SUPPORT_PVD=1"       or "PX_SUPPORT_PVD=0",
        USE_CHARACTER and "PX_ENABLE_CHARACTER=1"  or "PX_ENABLE_CHARACTER=0",
        USE_VEHICLE   and "PX_ENABLE_VEHICLE=1"    or "PX_ENABLE_VEHICLE=0",
    }

    filter "system:windows"
        -- Force MBCS so WinAPI uses narrow-char (avoids LPCWSTR issues).
        characterset "MBCS"
        defines { 'PX_PHYSX_GPU_SHARED_LIB_NAME="PhysXGpu_64.dll"' }
    filter {}

    filter { "system:linux or system:bsd" }
        defines { 'PX_PHYSX_GPU_SHARED_LIB_NAME="libPhysXGpu_64.so"' }
    filter {}
end

local function physx_platform()
    filter "system:windows"
        systemversion "latest"
        defines { "_CRT_SECURE_NO_WARNINGS", "WIN32", "_WINDOWS" }
    filter {}

    filter { "system:linux or system:bsd" }
        pic "On"
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

        physx_common_defines()
        physx_platform()
        physx_cfg()
end

-- ============================================================================
-- Foundation
-- ============================================================================
group "Dependencies/PhysX/Foundation"

new_physx_lib("PhysXFoundation")
files {
    PHYSX_ROOT .. "/include/foundation/**.h",
    PHYSX_ROOT .. "/source/foundation/FdAllocator.cpp",
    PHYSX_ROOT .. "/source/foundation/FdString.cpp",
    PHYSX_ROOT .. "/source/foundation/FdTempAllocator.cpp",
    PHYSX_ROOT .. "/source/foundation/FdAssert.cpp",
    PHYSX_ROOT .. "/source/foundation/FdMathUtils.cpp",
    PHYSX_ROOT .. "/source/foundation/FdFoundation.cpp",
    PHYSX_ROOT .. "/source/foundation/FdFoundation.h",
}
includedirs { PHYSX_ROOT .. "/include" }

filter "system:windows"
    files {
        PHYSX_ROOT .. "/include/foundation/windows/**.h",
        PHYSX_ROOT .. "/source/foundation/windows/**.cpp",
        PHYSX_ROOT .. "/source/compiler/windows/resource/PhysXFoundation.rc",
        PHYSX_ROOT .. "/source/compiler/windows/resource/resource.h"
    }
    defines { "_WINSOCK_DEPRECATED_NO_WARNINGS" }
    links { "Advapi32", "Ws2_32" }
filter {}

filter { "system:linux or system:bsd" }
    files {
        PHYSX_ROOT .. "/include/foundation/unix/**.h",
        PHYSX_ROOT .. "/source/foundation/unix/**.cpp"
    }
    defines { "_GNU_SOURCE" }
    links { "pthread", "dl", "m", "rt" }
filter {}

-- ============================================================================
-- Common + GeomUtils
-- ============================================================================
new_physx_lib("PhysXCommon")
files {
    PHYSX_ROOT .. "/include/common/**.h",
    PHYSX_ROOT .. "/include/geometry/**.h",
    PHYSX_ROOT .. "/include/geomutils/**.h",
    PHYSX_ROOT .. "/include/collision/**.h",

    PHYSX_ROOT .. "/source/common/src/**.cpp",
    PHYSX_ROOT .. "/source/common/src/**.h",

    PHYSX_ROOT .. "/source/geomutils/src/*.cpp",
    PHYSX_ROOT .. "/source/geomutils/src/*.h",
    PHYSX_ROOT .. "/source/geomutils/src/ccd/**.cpp",
    PHYSX_ROOT .. "/source/geomutils/src/ccd/**.h",
    PHYSX_ROOT .. "/source/geomutils/src/common/**.cpp",
    PHYSX_ROOT .. "/source/geomutils/src/common/**.h",
    PHYSX_ROOT .. "/source/geomutils/src/contact/**.cpp",
    PHYSX_ROOT .. "/source/geomutils/src/contact/**.h",
    PHYSX_ROOT .. "/source/geomutils/src/convex/**.cpp",
    PHYSX_ROOT .. "/source/geomutils/src/convex/**.h",
    PHYSX_ROOT .. "/source/geomutils/src/distance/**.cpp",
    PHYSX_ROOT .. "/source/geomutils/src/distance/**.h",
    PHYSX_ROOT .. "/source/geomutils/src/gjk/**.cpp",
    PHYSX_ROOT .. "/source/geomutils/src/gjk/**.h",
    PHYSX_ROOT .. "/source/geomutils/src/hf/**.cpp",
    PHYSX_ROOT .. "/source/geomutils/src/hf/**.h",
    PHYSX_ROOT .. "/source/geomutils/src/intersection/**.cpp",
    PHYSX_ROOT .. "/source/geomutils/src/intersection/**.h",
    PHYSX_ROOT .. "/source/geomutils/src/mesh/**.cpp",
    PHYSX_ROOT .. "/source/geomutils/src/mesh/**.h",
    PHYSX_ROOT .. "/source/geomutils/src/pcm/**.cpp",
    PHYSX_ROOT .. "/source/geomutils/src/pcm/**.h",
    PHYSX_ROOT .. "/source/geomutils/src/pruners/**.cpp",
    PHYSX_ROOT .. "/source/geomutils/src/pruners/**.h",
    PHYSX_ROOT .. "/source/geomutils/src/sweep/**.cpp",
    PHYSX_ROOT .. "/source/geomutils/src/sweep/**.h",
    PHYSX_ROOT .. "/source/geomutils/src/Gu*.h",
}
includedirs {
    PHYSX_ROOT .. "/include",
    PHYSX_ROOT .. "/source/common/include",
    PHYSX_ROOT .. "/source/common/src",
    PHYSX_ROOT .. "/source/geomutils/include",
    PHYSX_ROOT .. "/source/geomutils/src",
    PHYSX_ROOT .. "/source/geomutils/src/contact",
    PHYSX_ROOT .. "/source/geomutils/src/common",
    PHYSX_ROOT .. "/source/geomutils/src/convex",
    PHYSX_ROOT .. "/source/geomutils/src/distance",
    PHYSX_ROOT .. "/source/geomutils/src/sweep",
    PHYSX_ROOT .. "/source/geomutils/src/gjk",
    PHYSX_ROOT .. "/source/geomutils/src/intersection",
    PHYSX_ROOT .. "/source/geomutils/src/mesh",
    PHYSX_ROOT .. "/source/geomutils/src/hf",
    PHYSX_ROOT .. "/source/geomutils/src/pcm",
    PHYSX_ROOT .. "/source/geomutils/src/ccd",
    PHYSX_ROOT .. "/source/physxgpu/include",
}
links { "PhysXFoundation" }

filter "system:windows"
    files {
        PHYSX_ROOT .. "/source/common/include/windows/CmWindowsLoadLibrary.h",
        PHYSX_ROOT .. "/source/common/include/windows/CmWindowsModuleUpdateLoader.h",
        PHYSX_ROOT .. "/source/common/src/windows/CmWindowsModuleUpdateLoader.cpp",
        PHYSX_ROOT .. "/source/common/src/windows/CmWindowsDelayLoadHook.cpp",
        PHYSX_ROOT .. "/source/compiler/windows/resource/PhysXCommon.rc",
        PHYSX_ROOT .. "/source/compiler/windows/resource/resource.h"
    }
    includedirs { PHYSX_ROOT .. "/source/common/src/windows" }
    linkoptions { "/MAP" }
filter {}

filter { "system:linux or system:bsd" }
    includedirs { PHYSX_ROOT .. "/source/common/src/linux" }
    defines { "PX_PHYSX_STATIC_LIB" }
    links { "pthread", "dl", "m", "rt" }
filter {}

-- ============================================================================
-- Core PhysX
-- ============================================================================
group "Dependencies/PhysX/Core"

new_physx_lib("PhysX")
files {
    PHYSX_ROOT .. "/include/gpu/PxGpu.h",
    PHYSX_ROOT .. "/include/gpu/PxPhysicsGpu.h",
    PHYSX_ROOT .. "/include/cudamanager/PxCudaContextManager.h",
    PHYSX_ROOT .. "/include/cudamanager/PxCudaContext.h",
    PHYSX_ROOT .. "/include/cudamanager/PxCudaTypes.h",

    PHYSX_ROOT .. "/source/physx/src/gpu/PxGpu.cpp",
    PHYSX_ROOT .. "/source/physx/src/gpu/PxPhysXGpuModuleLoader.cpp",
}
includedirs {
    PHYSX_ROOT .. "/include",
    PHYSX_ROOT .. "/source/common/include",
    PHYSX_ROOT .. "/source/physxgpu/include",
}
links {
    "LowLevel",
    "LowLevelAABB",
    "LowLevelDynamics",
    "PhysXTask",
    "SceneQuery",
    "SimulationController",
    "PhysXCommon",
    "PhysXFoundation",
}
defines { "PX_PHYSX_STATIC_LIB" }

if not PUBLIC_RELEASE then
    files { PHYSX_ROOT .. "/source/physx/src/internal/device/PhysXIndicator.h" }
end

filter "system:windows"
    files {
        PHYSX_ROOT .. "/source/compiler/windows/resource/PhysX.rc",
        PHYSX_ROOT .. "/source/compiler/windows/resource/resource.h",
        PHYSX_ROOT .. "/include/common/windows/PxWindowsDelayLoadHook.h",
        PHYSX_ROOT .. "/source/physx/src/windows/NpWindowsDelayLoadHook.cpp"
    }
    if not PUBLIC_RELEASE then
        files {
            PHYSX_ROOT .. "/source/physx/src/internal/device/nvPhysXtoDrv.h",
            PHYSX_ROOT .. "/source/physx/src/internal/device/windows/PhysXIndicatorWindows.cpp",
        }
        includedirs { PHYSX_ROOT .. "/source/physx/src/internal/device" }
    end
    linkoptions { "/MAP" }
    if USE_CUDA_DELAYLOAD then
        linkoptions { "/DELAYLOAD:nvcuda.dll" }
    end
filter {}

filter { "system:linux or system:bsd" }
    if not PUBLIC_RELEASE then
        files { PHYSX_ROOT .. "/source/physx/src/internal/device/linux/PhysXIndicatorLinux.cpp" }
        includedirs { PHYSX_ROOT .. "/source/physx/src/internal/device" }
    end
    links { "dl" }
filter {}

-- ============================================================================
-- SceneQuery
-- ============================================================================
new_physx_lib("SceneQuery")
files {
    PHYSX_ROOT .. "/source/scenequery/include/SqFactory.h",
    PHYSX_ROOT .. "/source/scenequery/include/SqPruner.h",
    PHYSX_ROOT .. "/source/scenequery/include/SqPrunerData.h",
    PHYSX_ROOT .. "/source/scenequery/include/SqManager.h",
    PHYSX_ROOT .. "/source/scenequery/include/SqQuery.h",
    PHYSX_ROOT .. "/source/scenequery/include/SqTypedef.h",

    PHYSX_ROOT .. "/source/scenequery/src/SqFactory.cpp",
    PHYSX_ROOT .. "/source/scenequery/src/SqCompoundPruner.cpp",
    PHYSX_ROOT .. "/source/scenequery/src/SqCompoundPruningPool.cpp",
    PHYSX_ROOT .. "/source/scenequery/src/SqManager.cpp",
    PHYSX_ROOT .. "/source/scenequery/src/SqQuery.cpp",
    PHYSX_ROOT .. "/source/scenequery/src/SqCompoundPruner.h",
    PHYSX_ROOT .. "/source/scenequery/src/SqCompoundPruningPool.h",
}
includedirs {
    PHYSX_ROOT .. "/include",
    PHYSX_ROOT .. "/source/common/include",
    PHYSX_ROOT .. "/source/common/src",
    PHYSX_ROOT .. "/source/geomutils/include",
    PHYSX_ROOT .. "/source/geomutils/src",
    PHYSX_ROOT .. "/source/geomutils/src/contact",
    PHYSX_ROOT .. "/source/geomutils/src/common",
    PHYSX_ROOT .. "/source/geomutils/src/convex",
    PHYSX_ROOT .. "/source/geomutils/src/distance",
    PHYSX_ROOT .. "/source/geomutils/src/sweep",
    PHYSX_ROOT .. "/source/geomutils/src/gjk",
    PHYSX_ROOT .. "/source/geomutils/src/intersection",
    PHYSX_ROOT .. "/source/geomutils/src/mesh",
    PHYSX_ROOT .. "/source/geomutils/src/hf",
    PHYSX_ROOT .. "/source/geomutils/src/pcm",
    PHYSX_ROOT .. "/source/geomutils/src/ccd",
    PHYSX_ROOT .. "/source/scenequery/include",
    PHYSX_ROOT .. "/source/pvd/include",
}
filter "system:windows"
    defines { "PX_WINDOWS", "PX_PHYSX_STATIC_LIB" }
filter {}
filter { "system:linux or system:bsd" }
    defines { "PX_LINUX", "PX_PHYSX_STATIC_LIB" }
filter {}

-- ============================================================================
-- SimulationController
-- ============================================================================
new_physx_lib("SimulationController")
files {
    PHYSX_ROOT .. "/source/simulationcontroller/include/*.h",
    PHYSX_ROOT .. "/source/simulationcontroller/src/*.h",
    PHYSX_ROOT .. "/source/simulationcontroller/src/*.cpp",
}
includedirs {
    PHYSX_ROOT .. "/include",
    PHYSX_ROOT .. "/source/common/include",
    PHYSX_ROOT .. "/source/common/src",
    PHYSX_ROOT .. "/source/physxgpu/include",
    PHYSX_ROOT .. "/source/geomutils/include",
    PHYSX_ROOT .. "/source/geomutils/src",
    PHYSX_ROOT .. "/source/geomutils/src/contact",
    PHYSX_ROOT .. "/source/geomutils/src/common",
    PHYSX_ROOT .. "/source/geomutils/src/convex",
    PHYSX_ROOT .. "/source/geomutils/src/distance",
    PHYSX_ROOT .. "/source/geomutils/src/sweep",
    PHYSX_ROOT .. "/source/geomutils/src/gjk",
    PHYSX_ROOT .. "/source/geomutils/src/intersection",
    PHYSX_ROOT .. "/source/geomutils/src/mesh",
    PHYSX_ROOT .. "/source/geomutils/src/hf",
    PHYSX_ROOT .. "/source/geomutils/src/pcm",
    PHYSX_ROOT .. "/source/geomutils/src/ccd",
    PHYSX_ROOT .. "/source/simulationcontroller/include",
    PHYSX_ROOT .. "/source/simulationcontroller/src",
    PHYSX_ROOT .. "/source/lowlevel/api/include",
    PHYSX_ROOT .. "/source/lowlevel/common/include",
    PHYSX_ROOT .. "/source/lowlevel/common/include/collision",
    PHYSX_ROOT .. "/source/lowlevel/common/include/pipeline",
    PHYSX_ROOT .. "/source/lowlevel/common/include/utils",
    PHYSX_ROOT .. "/source/lowlevel/software/include",
    PHYSX_ROOT .. "/source/lowleveldynamics/include",
    PHYSX_ROOT .. "/source/lowleveldynamics/shared",
    PHYSX_ROOT .. "/source/lowlevelaabb/include",
}
filter "system:windows"
    includedirs {
        PHYSX_ROOT .. "/source/Common/src/windows",
        PHYSX_ROOT .. "/source/LowLevel/windows/include",
    }
filter {}
filter { "system:linux or system:bsd" }
    includedirs {
        PHYSX_ROOT .. "/source/common/src/linux",
        PHYSX_ROOT .. "/source/lowlevel/linux/include",
    }
    defines { "PX_PHYSX_STATIC_LIB" }
filter {}

-- ============================================================================
-- Cooking
-- ============================================================================
group "Dependencies/PhysX/Extensions"

new_physx_lib("PhysXCooking")
files {
    PHYSX_ROOT .. "/include/cooking/**.h",
    PHYSX_ROOT .. "/source/physxcooking/src/Cooking.cpp",
    PHYSX_ROOT .. "/source/physxcooking/src/Cooking.h",
}
includedirs {
    PHYSX_ROOT .. "/include",
    PHYSX_ROOT .. "/source/common/include",
    PHYSX_ROOT .. "/source/common/src",
    PHYSX_ROOT .. "/source/geomutils/include",
    PHYSX_ROOT .. "/source/geomutils/src",
    PHYSX_ROOT .. "/source/geomutils/src/mesh",
}
links { "PhysXCommon", "PhysXFoundation" }
defines { "PX_COOKING" }

filter "system:windows"
    files {
        PHYSX_ROOT .. "/source/physxcooking/src/windows/WindowsCookingDelayLoadHook.cpp",
        PHYSX_ROOT .. "/source/compiler/windows/resource/PhysXCooking.rc",
        PHYSX_ROOT .. "/source/compiler/windows/resource/resource.h",
    }
    linkoptions { "/MAP" }
filter {}

filter { "system:linux or system:bsd" }
    defines { "PX_PHYSX_STATIC_LIB" }
    links { "pthread", "dl", "m", "rt" }
filter {}

-- ============================================================================
-- Extensions
-- ============================================================================
new_physx_lib("PhysXExtensions")
files {
    PHYSX_ROOT .. "/source/physxextensions/src/**.cpp",
    PHYSX_ROOT .. "/source/physxextensions/src/**.h",
    PHYSX_ROOT .. "/source/physxmetadata/extensions/src/**.cpp",
    PHYSX_ROOT .. "/source/physxmetadata/extensions/include/**.h",
    PHYSX_ROOT .. "/include/extensions/**.h",
    PHYSX_ROOT .. "/include/filebuf/**.h",
}
includedirs {
    PHYSX_ROOT .. "/include",
    PHYSX_ROOT .. "/source/common/include",
    PHYSX_ROOT .. "/source/common/src",
    PHYSX_ROOT .. "/source/geomutils/include",
    PHYSX_ROOT .. "/source/geomutils/src",
    PHYSX_ROOT .. "/source/geomutils/src/intersection",
    PHYSX_ROOT .. "/source/geomutils/src/mesh",
    PHYSX_ROOT .. "/source/physxmetadata/core/include",
    PHYSX_ROOT .. "/source/physxmetadata/extensions/include",
    PHYSX_ROOT .. "/source/physxextensions/src",
    PHYSX_ROOT .. "/source/physxextensions/src/serialization/Xml",
    PHYSX_ROOT .. "/source/physxextensions/src/serialization/Binary",
    PHYSX_ROOT .. "/source/physxextensions/src/serialization/File",
    PHYSX_ROOT .. "/source/physx/src",
    PHYSX_ROOT .. "/source/pvd/include",
    PHYSX_ROOT .. "/source/scenequery/include",
}
links { "PhysXFoundation", "PhysX" }
if USE_PVD then links { "PhysXPvdSDK" } end

filter "system:windows"
    includedirs {
        PHYSX_ROOT .. "/source/Common/src/windows",
        PHYSX_ROOT .. "/source/common/src/windows",
    }
filter {}

filter { "system:linux or system:bsd" }
    defines { "PX_PHYSX_STATIC_LIB" }
    includedirs {
        PHYSX_ROOT .. "/source/Common/src/linux",
        PHYSX_ROOT .. "/source/common/src/linux",
    }
    links { "pthread", "dl", "m", "rt" }
filter {}

filter { "system:linux or system:bsd", "files:**/omnipvd/OmniPvdPxExtensionsSampler.cpp" }
    buildoptions { "-fpermissive", "-Wno-error" }
filter {}

-- ============================================================================
-- Character Kinematic (optional)
-- ============================================================================
if USE_CHARACTER then
    new_physx_lib("PhysXCharacterKinematic")
    files {
        PHYSX_ROOT .. "/include/characterkinematic/**.h",
        PHYSX_ROOT .. "/source/physxcharacterkinematic/src/CctBoxController.cpp",
        PHYSX_ROOT .. "/source/physxcharacterkinematic/src/CctCapsuleController.cpp",
        PHYSX_ROOT .. "/source/physxcharacterkinematic/src/CctCharacterController.cpp",
        PHYSX_ROOT .. "/source/physxcharacterkinematic/src/CctCharacterControllerCallbacks.cpp",
        PHYSX_ROOT .. "/source/physxcharacterkinematic/src/CctCharacterControllerManager.cpp",
        PHYSX_ROOT .. "/source/physxcharacterkinematic/src/CctController.cpp",
        PHYSX_ROOT .. "/source/physxcharacterkinematic/src/CctObstacleContext.cpp",
        PHYSX_ROOT .. "/source/physxcharacterkinematic/src/CctSweptBox.cpp",
        PHYSX_ROOT .. "/source/physxcharacterkinematic/src/CctSweptCapsule.cpp",
        PHYSX_ROOT .. "/source/physxcharacterkinematic/src/CctSweptVolume.cpp",
        PHYSX_ROOT .. "/source/physxcharacterkinematic/src/**.h",
    }
    includedirs {
        PHYSX_ROOT .. "/include",
        PHYSX_ROOT .. "/source/common/src",
        PHYSX_ROOT .. "/source/geomutils/include",
    }
    links { "PhysXFoundation" }
end

-- ============================================================================
-- Vehicle2 (optional)
-- ============================================================================
if USE_VEHICLE then
    new_physx_lib("PhysXVehicle2")
    files {
        PHYSX_ROOT .. "/include/vehicle2/**.h",

        PHYSX_ROOT .. "/source/physxvehicle/src/commands/VhCommandHelpers.cpp",
        PHYSX_ROOT .. "/source/physxvehicle/src/drivetrain/VhDrivetrainFunctions.cpp",
        PHYSX_ROOT .. "/source/physxvehicle/src/drivetrain/VhDrivetrainHelpers.cpp",
        PHYSX_ROOT .. "/source/physxvehicle/src/physxActor/VhPhysXActorFunctions.cpp",
        PHYSX_ROOT .. "/source/physxvehicle/src/physxActor/VhPhysXActorHelpers.cpp",
        PHYSX_ROOT .. "/source/physxvehicle/src/physxConstraints/VhPhysXConstraintFunctions.cpp",
        PHYSX_ROOT .. "/source/physxvehicle/src/physxConstraints/VhPhysXConstraintHelpers.cpp",
        PHYSX_ROOT .. "/source/physxvehicle/src/physxRoadGeometry/VhPhysXRoadGeometryFunctions.cpp",
        PHYSX_ROOT .. "/source/physxvehicle/src/physxRoadGeometry/VhPhysXRoadGeometryHelpers.cpp",
        PHYSX_ROOT .. "/source/physxvehicle/src/rigidBody/VhRigidBodyFunctions.cpp",
        PHYSX_ROOT .. "/source/physxvehicle/src/steering/VhSteeringFunctions.cpp",
        PHYSX_ROOT .. "/source/physxvehicle/src/suspension/VhSuspensionFunctions.cpp",
        PHYSX_ROOT .. "/source/physxvehicle/src/suspension/VhSuspensionHelpers.cpp",
        PHYSX_ROOT .. "/source/physxvehicle/src/tire/VhTireFunctions.cpp",
        PHYSX_ROOT .. "/source/physxvehicle/src/tire/VhTireHelpers.cpp",
        PHYSX_ROOT .. "/source/physxvehicle/src/wheel/VhWheelFunctions.cpp",

        PHYSX_ROOT .. "/source/physxvehicle/src/pvd/VhPvdHelpers.cpp",
        PHYSX_ROOT .. "/source/physxvehicle/src/pvd/VhPvdFunctions.cpp",
        PHYSX_ROOT .. "/source/physxvehicle/src/pvd/VhPvdWriter.cpp",
        PHYSX_ROOT .. "/source/physxvehicle/src/pvd/VhPvdAttributeHandles.h",
        PHYSX_ROOT .. "/source/physxvehicle/src/pvd/VhPvdObjectHandles.h",
        PHYSX_ROOT .. "/source/physxvehicle/src/pvd/VhPvdWriter.h",
    }
    includedirs {
        PHYSX_ROOT .. "/include",
        PHYSX_ROOT .. "/pvdruntime/include",
    }
    links { "PhysXFoundation", "PhysXCommon", "PhysXCooking", "PhysX", "PhysXExtensions" }
    defines { "PX_PHYSX_STATIC_LIB" }

    filter { "system:linux or system:bsd" }
        links { "pthread", "dl", "m", "rt" }
    filter {}
end

-- ============================================================================
-- PVD SDK (optional)
-- ============================================================================
group "Dependencies/PhysX/Tools"

if USE_PVD then
    new_physx_lib("PhysXPvdSDK")
    files {
        PHYSX_ROOT .. "/include/pvd/PxPvd.h",
        PHYSX_ROOT .. "/include/pvd/PxPvdTransport.h",

        PHYSX_ROOT .. "/source/pvd/src/PxProfileEventImpl.cpp",
        PHYSX_ROOT .. "/source/pvd/src/PxPvd.cpp",
        PHYSX_ROOT .. "/source/pvd/src/PxPvdDataStream.cpp",
        PHYSX_ROOT .. "/source/pvd/src/PxPvdDefaultFileTransport.cpp",
        PHYSX_ROOT .. "/source/pvd/src/PxPvdDefaultSocketTransport.cpp",
        PHYSX_ROOT .. "/source/pvd/src/PxPvdImpl.cpp",
        PHYSX_ROOT .. "/source/pvd/src/PxPvdMemClient.cpp",
        PHYSX_ROOT .. "/source/pvd/src/PxPvdObjectModelMetaData.cpp",
        PHYSX_ROOT .. "/source/pvd/src/PxPvdObjectRegistrar.cpp",
        PHYSX_ROOT .. "/source/pvd/src/PxPvdProfileZoneClient.cpp",
        PHYSX_ROOT .. "/source/pvd/src/PxPvdUserRenderer.cpp",
        PHYSX_ROOT .. "/source/pvd/src/**.h",

        PHYSX_ROOT .. "/source/pvd/include/**.h",
        PHYSX_ROOT .. "/source/filebuf/include/PsFileBuffer.h",
    }
    includedirs {
        PHYSX_ROOT .. "/include",
        PHYSX_ROOT .. "/source/pvd/include",
        PHYSX_ROOT .. "/source/filebuf/include",
    }
    defines { "PX_PHYSX_STATIC_LIB" }
    links { "PhysXFoundation", "PhysX" }

    filter { "system:linux or system:bsd" }
        links { "pthread", "dl", "m" }
    filter {}
end

-- ============================================================================
-- Task
-- ============================================================================
group "Dependencies/PhysX/LowLevel"

new_physx_lib("PhysXTask")
files {
    PHYSX_ROOT .. "/include/task/PxCpuDispatcher.h",
    PHYSX_ROOT .. "/include/task/PxTask.h",
    PHYSX_ROOT .. "/include/task/PxTaskManager.h",
    PHYSX_ROOT .. "/source/task/src/TaskManager.cpp",
}
includedirs {
    PHYSX_ROOT .. "/include",
    PHYSX_ROOT .. "/source/cudamanager/include",
}
filter "system:windows"
    defines { "_LIB" }
filter {}

-- ============================================================================
-- LowLevel
-- ============================================================================
new_physx_lib("LowLevel")
files {
    PHYSX_ROOT .. "/source/lowlevel/api/include/**.h",
    PHYSX_ROOT .. "/source/lowlevel/api/src/**.cpp",
    PHYSX_ROOT .. "/source/lowlevel/common/include/collision/**.h",
    PHYSX_ROOT .. "/source/lowlevel/common/include/pipeline/**.h",
    PHYSX_ROOT .. "/source/lowlevel/common/include/utils/**.h",
    PHYSX_ROOT .. "/source/lowlevel/common/src/pipeline/**.cpp",
    PHYSX_ROOT .. "/source/lowlevel/software/include/**.h",
    PHYSX_ROOT .. "/source/lowlevel/software/src/**.cpp",
}
includedirs {
    PHYSX_ROOT .. "/include",
    PHYSX_ROOT .. "/source/common/include",
    PHYSX_ROOT .. "/source/common/src",
    PHYSX_ROOT .. "/source/physxgpu/include",
    PHYSX_ROOT .. "/source/geomutils/include",
    PHYSX_ROOT .. "/source/geomutils/src",
    PHYSX_ROOT .. "/source/geomutils/src/contact",
    PHYSX_ROOT .. "/source/geomutils/src/common",
    PHYSX_ROOT .. "/source/geomutils/src/convex",
    PHYSX_ROOT .. "/source/geomutils/src/distance",
    PHYSX_ROOT .. "/source/geomutils/src/sweep",
    PHYSX_ROOT .. "/source/geomutils/src/gjk",
    PHYSX_ROOT .. "/source/geomutils/src/intersection",
    PHYSX_ROOT .. "/source/geomutils/src/mesh",
    PHYSX_ROOT .. "/source/geomutils/src/hf",
    PHYSX_ROOT .. "/source/geomutils/src/pcm",
    PHYSX_ROOT .. "/source/geomutils/src/ccd",
    PHYSX_ROOT .. "/source/lowlevel/api/include",
    PHYSX_ROOT .. "/source/lowlevel/common/include",
    PHYSX_ROOT .. "/source/lowlevel/common/include/collision",
    PHYSX_ROOT .. "/source/lowlevel/common/include/pipeline",
    PHYSX_ROOT .. "/source/lowlevel/common/include/utils",
    PHYSX_ROOT .. "/source/lowlevel/software/include",
    PHYSX_ROOT .. "/source/lowleveldynamics/include",
    PHYSX_ROOT .. "/source/lowleveldynamics/shared",
}
filter "system:windows"
    includedirs {
        PHYSX_ROOT .. "/source/Common/src/windows",
        PHYSX_ROOT .. "/source/LowLevel/software/include/windows",
        PHYSX_ROOT .. "/source/LowLevelDynamics/include/windows",
        PHYSX_ROOT .. "/source/LowLevel/common/include/pipeline/windows",
    }
    linkoptions { "/MAP" }
filter {}
filter { "system:linux or system:bsd" }
    includedirs {
        PHYSX_ROOT .. "/source/Common/src/linux",
        PHYSX_ROOT .. "/source/LowLevel/software/include/linux",
        PHYSX_ROOT .. "/source/LowLevelDynamics/include/linux",
        PHYSX_ROOT .. "/source/LowLevel/common/include/pipeline/linux",
    }
    defines { "PX_PHYSX_STATIC_LIB" }
filter {}
if _OPTIONS and _OPTIONS["use_gpu_static"] then
    defines { "PX_PHYSX_GPU_STATIC" }
end

-- ============================================================================
-- LowLevelAABB
-- ============================================================================
new_physx_lib("LowLevelAABB")
files {
    PHYSX_ROOT .. "/source/lowlevelaabb/include/**.h",
    PHYSX_ROOT .. "/source/lowlevelaabb/src/**.cpp",
    PHYSX_ROOT .. "/source/lowlevelaabb/src/**.h",
}
includedirs {
    PHYSX_ROOT .. "/include",
    PHYSX_ROOT .. "/source/common/include",
    PHYSX_ROOT .. "/source/common/src",
    PHYSX_ROOT .. "/source/geomutils/include",
    PHYSX_ROOT .. "/source/geomutils/src",
    PHYSX_ROOT .. "/source/lowlevel/api/include",
    PHYSX_ROOT .. "/source/lowlevel/common/include/utils",
    PHYSX_ROOT .. "/source/lowlevel/common/include/pipeline",
    PHYSX_ROOT .. "/source/lowlevelaabb/include",
    PHYSX_ROOT .. "/source/lowlevelaabb/src",
}
filter "system:windows"
    includedirs {
        PHYSX_ROOT .. "/source/Common/src/windows",
        PHYSX_ROOT .. "/source/LowLevelAABB/windows/include",
        PHYSX_ROOT .. "/source/GpuBroadPhase/include",
        PHYSX_ROOT .. "/source/GpuBroadPhase/src",
    }
    linkoptions { "/MAP" }
filter {}
filter { "system:linux or system:bsd" }
    includedirs {
        PHYSX_ROOT .. "/source/Common/src/linux",
        PHYSX_ROOT .. "/source/LowLevelAABB/linux/include",
        PHYSX_ROOT .. "/source/GpuBroadPhase/include",
        PHYSX_ROOT .. "/source/GpuBroadPhase/src",
    }
    defines { "PX_PHYSX_STATIC_LIB" }
filter {}
if _OPTIONS and _OPTIONS["use_gpu_static"] then
    defines { "PX_PHYSX_GPU_STATIC" }
end

-- ============================================================================
-- LowLevelDynamics
-- ============================================================================
new_physx_lib("LowLevelDynamics")
files {
    PHYSX_ROOT .. "/source/lowleveldynamics/include/**.h",
    PHYSX_ROOT .. "/source/lowleveldynamics/shared/**.h",
    PHYSX_ROOT .. "/source/lowleveldynamics/src/**.h",
    PHYSX_ROOT .. "/source/lowleveldynamics/src/**.cpp",
}
includedirs {
    PHYSX_ROOT .. "/include",
    PHYSX_ROOT .. "/source/common/src",
    PHYSX_ROOT .. "/source/geomutils/include",
    PHYSX_ROOT .. "/source/geomutils/src",
    PHYSX_ROOT .. "/source/geomutils/src/contact",
    PHYSX_ROOT .. "/source/lowlevel/api/include",
    PHYSX_ROOT .. "/source/lowlevel/common/include",
    PHYSX_ROOT .. "/source/lowlevel/common/include/pipeline",
    PHYSX_ROOT .. "/source/lowlevel/common/include/utils",
    PHYSX_ROOT .. "/source/lowlevel/software/include",
    PHYSX_ROOT .. "/source/lowleveldynamics/include",
    PHYSX_ROOT .. "/source/lowleveldynamics/shared",
    PHYSX_ROOT .. "/source/lowleveldynamics/src",
    PHYSX_ROOT .. "/source/physxgpu/include",
}
filter "system:windows"
    includedirs {
        PHYSX_ROOT .. "/source/common/src/windows",
        PHYSX_ROOT .. "/source/lowlevel/software/include/windows",
        PHYSX_ROOT .. "/source/lowleveldynamics/include/windows",
        PHYSX_ROOT .. "/source/lowlevel/common/include/pipeline/windows",
    }
filter {}
filter { "system:linux or system:bsd" }
    includedirs {
        PHYSX_ROOT .. "/source/common/src/linux",
        PHYSX_ROOT .. "/source/lowlevel/software/include/linux",
        PHYSX_ROOT .. "/source/lowleveldynamics/include/linux",
        PHYSX_ROOT .. "/source/lowlevel/common/include/pipeline/linux",
    }
    defines { "PX_PHYSX_STATIC_LIB" }
filter {}
if _OPTIONS and _OPTIONS["use_gpu_static"] then
    defines { "PX_PHYSX_GPU_STATIC" }
end

-- ============================================================================
-- Aggregate target: build everything in one click (no binary produced)
-- ============================================================================
do
    local allDeps = {
        "PhysXFoundation",
        "PhysXCommon",
        "LowLevel",
        "LowLevelAABB",
        "LowLevelDynamics",
        "PhysXTask",
        "SceneQuery",
        "SimulationController",
        "PhysX",
        "PhysXCooking",
        "PhysXExtensions",
    }
    if USE_CHARACTER then table.insert(allDeps, "PhysXCharacterKinematic") end
    if USE_VEHICLE   then table.insert(allDeps, "PhysXVehicle2")            end
    if USE_PVD       then table.insert(allDeps, "PhysXPvdSDK")              end

	group "Dependencies/PhysX"

    project "PhysX-All"
        kind "Utility"
        language "C++"
        targetdir ("bin/" .. OUTPUT_DIR .. "/%{prj.name}")
        objdir    ("bin-int/" .. OUTPUT_DIR .. "/%{prj.name}")

        dependson(allDeps)

        physx_common_defines()
        physx_platform()
        physx_cfg()
end

project "QuasarEngine-Core"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")
	
	pchheader "qepch.h"
    pchsource "src/qepch.cpp"

	files
	{
		"src/**.h",
		"src/**.hpp",
		"src/**.cpp"
	}

	includedirs
	{
		"src",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.Glad}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.assimp}",
		"%{IncludeDir.stb}",
		"%{IncludeDir.ImGuizmo}",
		"%{IncludeDir.mbedtls}",
		"%{IncludeDir.zlib}",
		"%{IncludeDir.entt}",
		"%{IncludeDir.yaml_cpp}",
		"%{IncludeDir.tinyfiledialogs}",
		"%{IncludeDir.VulkanSDK}",
		"%{IncludeDir.sol2}",
		"%{IncludeDir.lua}",
		"%{IncludeDir.PhysX}"
	}
	
	libdirs
	{
		"%{LibraryDir.VulkanSDK}"
	}
	
	defines
	{
		"GLFW_INCLUDE_NONE"
	}

	links
	{
		"GLFW",
		"Glad",
		"assimp",
		"ImGui",
		"yaml-cpp",
		"zlib",
		"ImGuizmo",
		"tinyfiledialogs",
		"mbedtls",
		"lua",
		"stb",
		
		-- PhysX modules
		-- "PhysXExtensions",
		-- "PhysXCooking",
		-- "PhysXCharacterKinematic",
		-- "PhysXVehicle2",
		-- "PhysX",
		-- "SimulationController",
		-- "SceneQuery",
		-- "PhysXTask",
		-- "LowLevelDynamics",
		-- "LowLevelAABB",
		-- "LowLevel",
		-- "PhysXCommon",
		-- "PhysXFoundation",
		
		"%{LibraryDir.PhysX}/PhysXExtensions_static_64.lib",
		"%{LibraryDir.PhysX}/PhysXCooking_64.lib",
		"%{LibraryDir.PhysX}/PhysXCharacterKinematic_static_64.lib",
		"%{LibraryDir.PhysX}/PhysXVehicle2_static_64.lib",
		"%{LibraryDir.PhysX}/PhysX_64.lib",
		"%{LibraryDir.PhysX}/SimulationController_static_64.lib",
		"%{LibraryDir.PhysX}/SceneQuery_static_64.lib",
		"%{LibraryDir.PhysX}/PhysXTask_static_64.lib",
		"%{LibraryDir.PhysX}/LowLevelDynamics_static_64.lib",
		"%{LibraryDir.PhysX}/LowLevelAABB_static_64.lib",
		"%{LibraryDir.PhysX}/LowLevel_static_64.lib",
		"%{LibraryDir.PhysX}/PhysXCommon_64.lib",
		"%{LibraryDir.PhysX}/PhysXFoundation_64.lib",
		"%{LibraryDir.PhysX}/PhysXPvdSDK_static_64.lib",
		"%{LibraryDir.PhysX}/PVDRuntime_64.lib",
		
		"%{Library.Vulkan}" --,
		-- "%{Library.VulkanUtils}"
	}
	
	filter "toolset:msc*"
		buildoptions { "/bigobj" }

	filter "system:windows"
		systemversion "latest"
		
		links
		{
			"d3d11",
			"dxgi",
			"dxguid"
		}

	filter "configurations:Debug"
		defines { "DEBUG", "_DEBUG" }
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines { "RELEASE", "NDEBUG" }		
		runtime "Release"
		optimize "on"
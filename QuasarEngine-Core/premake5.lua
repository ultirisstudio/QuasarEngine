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
		
		--links
		--{			
		--	"%{LibraryDir.PhysX_Debug}/PhysXExtensions_static_64.lib",
		--	"%{LibraryDir.PhysX_Debug}/PhysXCooking_64.lib",
		--	"%{LibraryDir.PhysX_Debug}/PhysXCharacterKinematic_static_64.lib",
		--	"%{LibraryDir.PhysX_Debug}/PhysXVehicle2_static_64.lib",
		--	"%{LibraryDir.PhysX_Debug}/PhysX_64.lib",
		--	"%{LibraryDir.PhysX_Debug}/SimulationController_static_64.lib",
		--	"%{LibraryDir.PhysX_Debug}/SceneQuery_static_64.lib",
		--	"%{LibraryDir.PhysX_Debug}/PhysXTask_static_64.lib",
		--	"%{LibraryDir.PhysX_Debug}/LowLevelDynamics_static_64.lib",
		--	"%{LibraryDir.PhysX_Debug}/LowLevelAABB_static_64.lib",
		--	"%{LibraryDir.PhysX_Debug}/LowLevel_static_64.lib",
		--	"%{LibraryDir.PhysX_Debug}/PhysXCommon_64.lib",
		--	"%{LibraryDir.PhysX_Debug}/PhysXFoundation_64.lib",
		--	"%{LibraryDir.PhysX_Debug}/PhysXPvdSDK_static_64.lib",
		--	"%{LibraryDir.PhysX_Debug}/PVDRuntime_64.lib"
		--}

	filter "configurations:Release"
		defines { "RELEASE", "NDEBUG" }		
		runtime "Release"
		optimize "on"
		
		--links
		--{			
		--	"%{LibraryDir.PhysX_Realease}/PhysXExtensions_static_64.lib",
		--	"%{LibraryDir.PhysX_Realease}/PhysXCooking_64.lib",
		--	"%{LibraryDir.PhysX_Realease}/PhysXCharacterKinematic_static_64.lib",
		--	"%{LibraryDir.PhysX_Realease}/PhysXVehicle2_static_64.lib",
		--	"%{LibraryDir.PhysX_Realease}/PhysX_64.lib",
		--	"%{LibraryDir.PhysX_Realease}/SimulationController_static_64.lib",
		--	"%{LibraryDir.PhysX_Realease}/SceneQuery_static_64.lib",
		--	"%{LibraryDir.PhysX_Realease}/PhysXTask_static_64.lib",
		--	"%{LibraryDir.PhysX_Realease}/LowLevelDynamics_static_64.lib",
		--	"%{LibraryDir.PhysX_Realease}/LowLevelAABB_static_64.lib",
		--	"%{LibraryDir.PhysX_Realease}/LowLevel_static_64.lib",
		--	"%{LibraryDir.PhysX_Realease}/PhysXCommon_64.lib",
		--	"%{LibraryDir.PhysX_Realease}/PhysXFoundation_64.lib",
		--	"%{LibraryDir.PhysX_Realease}/PhysXPvdSDK_static_64.lib",
		--	"%{LibraryDir.PhysX_Realease}/PVDRuntime_64.lib"
		--}
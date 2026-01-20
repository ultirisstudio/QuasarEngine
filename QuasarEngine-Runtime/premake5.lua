project "QuasarEngine-Runtime"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"src/**.h",
		"src/**.cpp"
	}

	includedirs
	{
		"src",
		"%{IncludeDir.QuasarEngineCore}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.zlib}",
		"%{IncludeDir.Glad}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.yaml_cpp}",
		"%{IncludeDir.ImGuizmo}",
		"%{IncludeDir.tinyfiledialogs}",
		"%{IncludeDir.mbedtls}",
		"%{IncludeDir.zlib}",
		"%{IncludeDir.entt}",
		"%{IncludeDir.sol2}",
		"%{IncludeDir.lua}",
		"%{IncludeDir.PhysX}"
	}

	links
	{
		"QuasarEngine-Core",
		"tinyfiledialogs",
		"yaml-cpp",
		"mbedtls",
		"zlib",
		"lua",
		"Glad"
	}
	
	filter "system:windows"
		systemversion "latest"
	
	filter "configurations:Debug"
		defines { "DEBUG", "_DEBUG" }
		runtime "Debug"
		symbols "on"
		
		links
		{			
			"%{LibraryDir.PhysX_Debug}/PhysXExtensions_static_64.lib",
			"%{LibraryDir.PhysX_Debug}/PhysXCooking_64.lib",
			"%{LibraryDir.PhysX_Debug}/PhysXCharacterKinematic_static_64.lib",
			"%{LibraryDir.PhysX_Debug}/PhysXVehicle2_static_64.lib",
			"%{LibraryDir.PhysX_Debug}/PhysX_64.lib",
			"%{LibraryDir.PhysX_Debug}/SimulationController_static_64.lib",
			"%{LibraryDir.PhysX_Debug}/SceneQuery_static_64.lib",
			"%{LibraryDir.PhysX_Debug}/PhysXTask_static_64.lib",
			"%{LibraryDir.PhysX_Debug}/LowLevelDynamics_static_64.lib",
			"%{LibraryDir.PhysX_Debug}/LowLevelAABB_static_64.lib",
			"%{LibraryDir.PhysX_Debug}/LowLevel_static_64.lib",
			"%{LibraryDir.PhysX_Debug}/PhysXCommon_64.lib",
			"%{LibraryDir.PhysX_Debug}/PhysXFoundation_64.lib",
			"%{LibraryDir.PhysX_Debug}/PhysXPvdSDK_static_64.lib",
			"%{LibraryDir.PhysX_Debug}/PVDRuntime_64.lib"
		}

	filter "configurations:Release"
		defines { "RELEASE", "NDEBUG" }	
		runtime "Release"
		optimize "on"
		
		links
		{			
			"%{LibraryDir.PhysX_Realease}/PhysXExtensions_static_64.lib",
			"%{LibraryDir.PhysX_Realease}/PhysXCooking_64.lib",
			"%{LibraryDir.PhysX_Realease}/PhysXCharacterKinematic_static_64.lib",
			"%{LibraryDir.PhysX_Realease}/PhysXVehicle2_static_64.lib",
			"%{LibraryDir.PhysX_Realease}/PhysX_64.lib",
			"%{LibraryDir.PhysX_Realease}/SimulationController_static_64.lib",
			"%{LibraryDir.PhysX_Realease}/SceneQuery_static_64.lib",
			"%{LibraryDir.PhysX_Realease}/PhysXTask_static_64.lib",
			"%{LibraryDir.PhysX_Realease}/LowLevelDynamics_static_64.lib",
			"%{LibraryDir.PhysX_Realease}/LowLevelAABB_static_64.lib",
			"%{LibraryDir.PhysX_Realease}/LowLevel_static_64.lib",
			"%{LibraryDir.PhysX_Realease}/PhysXCommon_64.lib",
			"%{LibraryDir.PhysX_Realease}/PhysXFoundation_64.lib",
			"%{LibraryDir.PhysX_Realease}/PhysXPvdSDK_static_64.lib",
			"%{LibraryDir.PhysX_Realease}/PVDRuntime_64.lib"
		}
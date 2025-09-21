project "QuasarEngine-Editor"
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
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.yaml_cpp}",
		"%{IncludeDir.ImGuizmo}",
		"%{IncludeDir.tinyfiledialogs}",
		"%{IncludeDir.mbedtls}",
		"%{IncludeDir.zlib}",
		"%{IncludeDir.entt}",
		"%{IncludeDir.TextEditor}",
		"%{IncludeDir.sol2}",
		"%{IncludeDir.lua}",
		"%{IncludeDir.PhysX}"
	}

	links
	{
		"QuasarEngine-Core",
		"tinyfiledialogs",
		"ImGui",
		"ImGuizmo",
		"yaml-cpp",
		"mbedtls",
		"zlib",
		"TextEditor",
		"lua",
		
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
	}
	
	defines
	{
		"IMGUI_DEFINE_MATH_OPERATORS"
	}
	
	filter "system:windows"
		systemversion "latest"
	
	filter "configurations:Debug"
		defines { "DEBUG", "_DEBUG" }
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines { "RELEASE", "NDEBUG" }	
		runtime "Release"
		optimize "on"
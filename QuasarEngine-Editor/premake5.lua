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
		"%{IncludeDir.PhysX}",
		"%{IncludeDir.PhysXsrc}"
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
		"PhysXExtensions",
		"PhysXCooking",
		"PhysXCharacterKinematic",
		"PhysXVehicle2",
		"PhysX",
		"SimulationController",
		"SceneQuery",
		"PhysXTask",
		"LowLevelDynamics",
		"LowLevelAABB",
		"LowLevel",
		"PhysXCommon",
		"PhysXFoundation"
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
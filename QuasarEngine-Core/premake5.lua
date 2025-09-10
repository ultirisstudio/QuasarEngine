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
		"src/**.cpp",
		
		"../vendor/stb_image/stb_image.h",
		"../vendor/stb_image/stb_image.cpp"
	}

	includedirs
	{
		"src",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.Glad}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.assimp}",
		"%{IncludeDir.stb_image}",
		"%{IncludeDir.ImGuizmo}",
		"%{IncludeDir.mbedtls}",
		"%{IncludeDir.entt}",
		"%{IncludeDir.VulkanSDK}",
		"%{IncludeDir.sol2}",
		"%{IncludeDir.lua}",
		"%{IncludeDir.PhysX}",
		"%{IncludeDir.PhysXsrc}"
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
		"ImGuizmo",
		"mbedtls",
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
		"PhysXFoundation",
		
		"%{Library.Vulkan}" --,
		-- "%{Library.VulkanUtils}"
	}

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
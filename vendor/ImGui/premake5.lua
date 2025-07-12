project "ImGui"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"imgui/**.h",
		"imgui/**.cpp",
		
		"backends/imgui_impl_opengl3.cpp",
		"backends/imgui_impl_opengl3.h",
		"backends/imgui_impl_vulkan.cpp",
		"backends/imgui_impl_vulkan.h",
		"backends/imgui_impl_glfw.cpp",
		"backends/imgui_impl_glfw.h"
	}
	
	includedirs
	{
		"imgui",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.VulkanSDK}",
		"%{IncludeDir.VulkanSDK}"
	}
	
	libdirs
	{
		"%{LibraryDir.VulkanSDK}"
	}
	
	links
	{
		"GLFW",
		
		"%{Library.Vulkan}"
	}

	filter "system:windows"
		systemversion "latest"

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		runtime "Release"
		optimize "on"

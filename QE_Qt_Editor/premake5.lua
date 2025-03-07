project "QE_Qt_Editor"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")
	
	filter "action:vs*"
        buildoptions { "/Zc:__cplusplus", "/permissive-" }

	files
	{
		"src/**.h",
		"src/**.hpp",
		"src/**.cpp"
	}

	includedirs
	{
		"%{IncludeDir.QE_Qt_Editor}",
		
		"%{IncludeDir.QE_Core}",
		"%{IncludeDir.QE_Vulkan_API}",
		"%{IncludeDir.QE_OpenGL_API}",
		"%{IncludeDir.QE_DirectX_API}",
		
		"%{IncludeDir.QE_ImGui_GUI}",
		"%{IncludeDir.QE_Qt_GUI}",
		
		"%{IncludeDir.QE_GLFW_Window}",
		"%{IncludeDir.QE_Qt_Window}",
		
		"%{IncludeDir.QT_SDK}",
		
		"%{IncludeDir.glm}",
		"%{IncludeDir.mbedtls}",
		"%{IncludeDir.stb_image}"
	}
	
	libdirs
	{
		"%{LibraryDir.QT_SDK}"
	}

	links
	{
		"QE_Core",
		
		"QE_Vulkan_API",
		"QE_OpenGL_API",
		"QE_DirectX_API",
		
		"QE_ImGui_GUI",
		"QE_Qt_GUI",
		
		"QE_GLFW_Window",
		"QE_Qt_Window",
		
		"mbedtls",
		"stb_image"
	}
	
	filter "system:windows"
		systemversion "latest"
	
	filter "configurations:Debug"
		defines "DEBUG"
		runtime "Debug"
		symbols "on"
		
		links
		{
			"Qt6Cored.lib",
			"Qt6Guid.lib",
			"Qt6Widgetsd.lib"
		}

	filter "configurations:Release"
		defines "RELEASE"
		runtime "Release"
		optimize "on"
		
		links
		{
			"Qt6Core.lib",
			"Qt6Gui.lib",
			"Qt6Widgets.lib"
		}
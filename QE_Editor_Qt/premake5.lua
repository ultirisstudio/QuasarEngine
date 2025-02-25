project "QE_Editor_Qt"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"src/**.h",
		"src/**.hpp",
		"src/**.cpp"
	}

	includedirs
	{
		"src",
		
		"%{IncludeDir.QE_Core}",
		"%{IncludeDir.QE_Vulkan_API}",
		"%{IncludeDir.QE_OpenGL_API}",
		"%{IncludeDir.QE_DirectX_API}"
	}

	links
	{
		"QE_Core",
		
		"QE_Vulkan_API",
		"QE_OpenGL_API",
		"QE_DirectX_API"
	}
	
	filter "system:windows"
		systemversion "latest"
	
	filter "configurations:Debug"
		defines "DEBUG"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines "RELEASE"
		runtime "Release"
		optimize "on"
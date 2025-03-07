project "QE_Core"
	kind "StaticLib"
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
		"%{IncludeDir.QE_Core}",
		
		"%{IncludeDir.glm}",
		"%{IncludeDir.mbedtls}",
		
		"%{IncludeDir.QT_SDK}"
	}
	
	libdirs
	{
		
	}
	
	defines
	{
		"GLFW_INCLUDE_NONE"
	}

	links
	{
		"mbedtls"
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
project "QE_Qt_Window"
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
		"%{IncludeDir.QE_Qt_Window}",
		
		"%{IncludeDir.QT_SDK}"
	}

	links
	{
		"QE_Core"
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
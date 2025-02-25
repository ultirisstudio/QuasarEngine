project "mbedtls"
	kind "StaticLib"
	language "C"
	staticruntime "on"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	defines { "_CRT_SECURE_NO_WARNINGS" }

	files
	{
		"include/**.h",
		"library/**.c"
	}
	
	includedirs
	{
		"include",
		"library"
	}

	filter "system:windows"
		systemversion "latest"

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		runtime "Release"
		optimize "on"

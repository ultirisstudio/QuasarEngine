project "tinyfiledialogs"
	kind "StaticLib"
	language "C"
	staticruntime "on"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	includedirs {
		"include"
	}

	files {
		"include/tinyfiledialogs/tinyfiledialogs.h",
		"src/tinyfiledialogs.c"
	}
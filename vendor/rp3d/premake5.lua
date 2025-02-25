project "reactphysics3d"
    kind "StaticLib"
    language "C++"
	cppdialect "C++17"
    staticruntime "On"
	
	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

    files {
        "include/**.h",
        "src/**.cpp"
    }
	
	includedirs {
		"include"
	}

    filter "system:windows"
        systemversion "latest"

    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        runtime "Release"
        optimize "on"
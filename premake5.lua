include "Dependencies.lua"

workspace "QuasarEngine"
	architecture "x86_64"
    startproject "QuasarEngine-Editor"

	configurations
	{
		"Debug",
		"Release"
	}
	
	flags
	{
		"MultiProcessorCompile"
	}
	
	filter "action:vs*"
		toolset "v145"
		systemversion "latest"

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

group "Dependencies"
	include "vendor/GLFW"
	include "vendor/Glad"
	include "vendor/assimp"
	include "vendor/ImGui"
	include "vendor/ImGuizmo"
	include "vendor/yaml_cpp"
	include "vendor/tinyfiledialogs"
	include "vendor/mbedtls"
	include "vendor/zlib"
	include "vendor/TextEditor"
	include "vendor/lua"
	include "vendor/stb"
	-- include "vendor/PhysX"
group ""

group "Core"
	include "QuasarEngine-Core"
group ""

group "Editor"
	include "QuasarEngine-Editor"
group ""

group "Runtime"
	include "QuasarEngine-Runtime"
group ""

group "Units"
	include "QuasarEngine-Units"
group ""


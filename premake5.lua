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

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

group "Dependance"
	include "vendor/GLFW"
	include "vendor/assimp"
	include "vendor/ImGui"
	include "vendor/ImGuizmo"
	include "vendor/yaml_cpp"
	include "vendor/tinyfiledialogs"
	include "vendor/mbedtls"
	include "vendor/zlib"
	include "vendor/TextEditor"

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
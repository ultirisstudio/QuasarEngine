include "Dependencies.lua"

workspace "QuasarEngine"
	architecture "x86_64"
    startproject "QE_Editor_ImGui"

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
	include "vendor/Glad"
	include "vendor/assimp"
	include "vendor/ImGui"
	include "vendor/ImGuizmo"
	include "vendor/yaml_cpp"
	include "vendor/tinyfiledialogs"
	include "vendor/rp3d"
	include "vendor/mbedtls"
	include "vendor/zlib"

group ""

group "CORE"
	include "QE_Core"
group ""

group "API"
	include "QE_Vulkan_API"
	include "QE_OpenGL_API"
	include "QE_DirectX_API"
group ""

group "EDITOR"
	include "QE_Editor_ImGui"
	include "QE_Editor_Qt"
group ""

group "Runtime"
	include "QE_Runtime"
group ""
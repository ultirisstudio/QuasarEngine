include "Dependencies.lua"

workspace "QuasarEngine"
	architecture "x86_64"
    startproject "QE_ImGui_Editor"

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

group "DEPENDANCE"
	include "vendor/GLFW"
	include "vendor/Glad"
	include "vendor/assimp"
	include "vendor/ImGui"
	include "vendor/ImGuizmo"
	include "vendor/yaml_cpp"
	include "vendor/stb_image"
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

group "GUI"
	include "QE_ImGui_GUI"
	include "QE_Qt_GUI"
group ""

group "WINDOW"
	include "QE_GLFW_Window"
	include "QE_Qt_Window"
group ""

group "EDITOR"
	include "QE_ImGui_Editor"
	include "QE_Qt_Editor"
group ""

group "RUNTIME"
	include "QE_Runtime"
group ""
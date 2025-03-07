VULKAN_SDK = os.getenv("VULKAN_SDK")
DIRECTX_SDK = os.getenv("DXSDK_DIR")

QT_SDK = os.getenv("QT_SDK")

IncludeDir = {}

IncludeDir["QE_Core"] = "%{wks.location}/QE_Core/src"

IncludeDir["QE_Vulkan_API"] = "%{wks.location}/QE_Vulkan_API/src"
IncludeDir["QE_OpenGL_API"] = "%{wks.location}/QE_OpenGL_API/src"
IncludeDir["QE_DirectX_API"] = "%{wks.location}/QE_DirectX_API/src"

IncludeDir["QE_Qt_GUI"] = "%{wks.location}/QE_Qt_GUI/src"
IncludeDir["QE_ImGui_GUI"] = "%{wks.location}/QE_ImGui_GUI/src"

IncludeDir["QE_GLFW_Window"] = "%{wks.location}/QE_GLFW_Window/src"
IncludeDir["QE_Qt_Window"] = "%{wks.location}/QE_Qt_Window/src"

IncludeDir["QE_Qt_Editor"] = "%{wks.location}/QE_Qt_Editor/src"
IncludeDir["QE_ImGui_Editor"] = "%{wks.location}/QE_ImGui_Editor/src"

IncludeDir["GLFW"] = "%{wks.location}/vendor/GLFW/include"
IncludeDir["Glad"] = "%{wks.location}/vendor/Glad/include"
IncludeDir["glm"] = "%{wks.location}/vendor/glm"
IncludeDir["filewatch"] = "%{wks.location}/vendor/filewatch"
IncludeDir["ImGui"] = "%{wks.location}/vendor/ImGui"
IncludeDir["assimp"] = "%{wks.location}/vendor/assimp/include"
IncludeDir["yaml_cpp"] = "%{wks.location}/vendor/yaml_cpp/include"
IncludeDir["stb_image"] = "%{wks.location}/vendor/stb_image/include"
IncludeDir["ImGuizmo"] = "%{wks.location}/vendor/ImGuizmo"
IncludeDir["tinyfiledialogs"] = "%{wks.location}/vendor/tinyfiledialogs/include"
IncludeDir["reactphysics3d"] = "%{wks.location}/vendor/rp3d/include"
IncludeDir["mbedtls"] = "%{wks.location}/vendor/mbedtls/include"
IncludeDir["zlib"] = "%{wks.location}/vendor/zlib/include"
IncludeDir["entt"] = "%{wks.location}/vendor/entt"
IncludeDir["tinygltf"] = "%{wks.location}/vendor/TinyGLTF"
IncludeDir["VulkanSDK"] = "%{VULKAN_SDK}/Include"
IncludeDir["DirectXSDK"] = "%{DIRECTX_SDK}/Include"
IncludeDir["QT_SDK"] = "%{QT_SDK}/include"

LibraryDir = {}
LibraryDir["VulkanSDK"] = "%{VULKAN_SDK}/Lib"
LibraryDir["DirectXSDK"] = "%{DIRECTX_SDK}/Lib/x64"
LibraryDir["QT_SDK"] = "%{QT_SDK}/lib"

Library = {}
Library["Vulkan"] = "%{LibraryDir.VulkanSDK}/vulkan-1.lib"
-- Library["VulkanUtils"] = "%{LibraryDir.VulkanSDK}/VkLayer_utils.lib"
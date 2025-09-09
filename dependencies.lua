VULKAN_SDK = os.getenv("VULKAN_SDK")

IncludeDir = {}
IncludeDir["QuasarEngineCore"] = "%{wks.location}/QuasarEngine-Core/src"
IncludeDir["GLFW"] = "%{wks.location}/vendor/GLFW/include"
IncludeDir["Glad"] = "%{wks.location}/vendor/Glad/include"
IncludeDir["glm"] = "%{wks.location}/vendor/glm"
IncludeDir["ImGui"] = "%{wks.location}/vendor/ImGui"
IncludeDir["assimp"] = "%{wks.location}/vendor/assimp/include"
IncludeDir["stb_image"] = "%{wks.location}/vendor/stb_image"
IncludeDir["yaml_cpp"] = "%{wks.location}/vendor/yaml_cpp/include"
IncludeDir["ImGuizmo"] = "%{wks.location}/vendor/ImGuizmo"
IncludeDir["tinyfiledialogs"] = "%{wks.location}/vendor/tinyfiledialogs/include"
IncludeDir["mbedtls"] = "%{wks.location}/vendor/mbedtls/include"
IncludeDir["zlib"] = "%{wks.location}/vendor/zlib/include"
IncludeDir["entt"] = "%{wks.location}/vendor/entt"
IncludeDir["TextEditor"] = "%{wks.location}/vendor/TextEditor"
IncludeDir["sol2"] = "%{wks.location}/vendor/sol2/include"
IncludeDir["lua"] = "%{wks.location}/vendor/lua/include"
IncludeDir["VulkanSDK"] = "%{VULKAN_SDK}/Include"
IncludeDir["PhysX"] = "%{wks.location}/vendor/PhysX/physx/include"

LibraryDir = {}
LibraryDir["VulkanSDK"] = "%{VULKAN_SDK}/Lib"

Library = {}
Library["Vulkan"] = "%{LibraryDir.VulkanSDK}/vulkan-1.lib"
-- Library["VulkanUtils"] = "%{LibraryDir.VulkanSDK}/VkLayer_utils.lib"
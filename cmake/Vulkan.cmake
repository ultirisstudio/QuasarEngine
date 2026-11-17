# Usage: install the Vulkan SDK, then configure as usual. Its installer sets the VULKAN_SDK environment variable, which
# find_package(Vulkan) reads to find the headers and the loader. On Linux, the libvulkan-dev package works too.
#
# Without the SDK, the engine is built without the Vulkan graphics API. Set QUASAR_VULKAN to ON to require it.

find_package(Vulkan 1.3)

option(QUASAR_VULKAN "Build the Vulkan graphics API, with the Vulkan SDK" ${Vulkan_FOUND})

if(QUASAR_VULKAN AND NOT Vulkan_FOUND)
    message(FATAL_ERROR "QUASAR_VULKAN needs the Vulkan SDK 1.3 or later. Install it, or check that the VULKAN_SDK "
                        "environment variable points to it.")
endif()

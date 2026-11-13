#include <quasar/platform/vulkan/VulkanContext.h>

#include <quasar/core/PlatformDetection.h>
#include <quasar/log/Log.h>

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_vulkan.h>

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <limits>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace Quasar
{
    namespace
    {
        constexpr std::uint32_t RequiredApiVersion = VK_API_VERSION_1_3;
        constexpr const char* ValidationLayer = "VK_LAYER_KHRONOS_validation";
        constexpr std::uint32_t PreferredImageCount = 3;

        const char* GetResultName(VkResult result)
        {
            switch (result)
            {
                case VK_ERROR_OUT_OF_HOST_MEMORY:
                    return "out of host memory";
                case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                    return "out of device memory";
                case VK_ERROR_INITIALIZATION_FAILED:
                    return "initialization failed";
                case VK_ERROR_DEVICE_LOST:
                    return "device lost";
                case VK_ERROR_LAYER_NOT_PRESENT:
                    return "layer not present";
                case VK_ERROR_EXTENSION_NOT_PRESENT:
                    return "extension not present";
                case VK_ERROR_FEATURE_NOT_PRESENT:
                    return "feature not present";
                case VK_ERROR_INCOMPATIBLE_DRIVER:
                    return "incompatible driver";
                case VK_ERROR_SURFACE_LOST_KHR:
                    return "surface lost";
                case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
                    return "native window in use";
                default:
                    return "unknown error";
            }
        }

        bool Succeeded(VkResult result, const char* action)
        {
            if (result == VK_SUCCESS)
            {
                return true;
            }
            QUASAR_ENGINE_ERROR("Vulkan cannot {}: {} ({})", action, GetResultName(result), static_cast<int>(result));
            return false;
        }

        VKAPI_ATTR VkBool32 VKAPI_CALL LogValidationMessage(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT /*types*/,
                                                            const VkDebugUtilsMessengerCallbackDataEXT* data, void* /*userData*/)
        {
            if ((severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) != 0)
            {
                QUASAR_ENGINE_ERROR("Vulkan validation: {}", data->pMessage);
            }
            else if ((severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) != 0)
            {
                QUASAR_ENGINE_WARN("Vulkan validation: {}", data->pMessage);
            }
            return VK_FALSE;
        }

        VkDebugUtilsMessengerCreateInfoEXT GetMessengerInfo()
        {
            VkDebugUtilsMessengerCreateInfoEXT info{};
            info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            info.messageType =
                VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            info.pfnUserCallback = LogValidationMessage;
            return info;
        }

        bool HasLayer(const char* name)
        {
            std::uint32_t count = 0;
            vkEnumerateInstanceLayerProperties(&count, nullptr);
            std::vector<VkLayerProperties> layers(count);
            vkEnumerateInstanceLayerProperties(&count, layers.data());
            return std::ranges::any_of(layers, [name](const VkLayerProperties& layer) { return std::string_view(std::data(layer.layerName)) == name; });
        }

        bool HasExtension(std::span<const VkExtensionProperties> extensions, const char* name)
        {
            return std::ranges::any_of(extensions,
                                       [name](const VkExtensionProperties& extension) { return std::string_view(std::data(extension.extensionName)) == name; });
        }

        std::vector<VkExtensionProperties> GetInstanceExtensions(const char* layer)
        {
            std::uint32_t count = 0;
            vkEnumerateInstanceExtensionProperties(layer, &count, nullptr);
            std::vector<VkExtensionProperties> extensions(count);
            vkEnumerateInstanceExtensionProperties(layer, &count, extensions.data());
            return extensions;
        }

        std::vector<VkExtensionProperties> GetDeviceExtensions(VkPhysicalDevice device)
        {
            std::uint32_t count = 0;
            vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);
            std::vector<VkExtensionProperties> extensions(count);
            vkEnumerateDeviceExtensionProperties(device, nullptr, &count, extensions.data());
            return extensions;
        }

        bool SupportsRequiredFeatures(VkPhysicalDevice device)
        {
            VkPhysicalDeviceVulkan13Features features13{};
            features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
            VkPhysicalDeviceFeatures2 features{};
            features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            features.pNext = &features13;
            vkGetPhysicalDeviceFeatures2(device, &features);
            return features13.dynamicRendering == VK_TRUE && features13.synchronization2 == VK_TRUE;
        }

        std::optional<std::uint32_t> FindQueueFamily(VkPhysicalDevice device, VkSurfaceKHR surface)
        {
            std::uint32_t count = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
            std::vector<VkQueueFamilyProperties> families(count);
            vkGetPhysicalDeviceQueueFamilyProperties(device, &count, families.data());

            for (std::uint32_t index = 0; index < count; ++index)
            {
                VkBool32 canPresent = VK_FALSE;
                vkGetPhysicalDeviceSurfaceSupportKHR(device, index, surface, &canPresent);
                if ((families.at(index).queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0 && canPresent == VK_TRUE)
                {
                    return index;
                }
            }
            return std::nullopt;
        }

        int GetDeviceScore(const VkPhysicalDeviceProperties& properties)
        {
            switch (properties.deviceType)
            {
                case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                    return 2;
                case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                    return 1;
                default:
                    return 0;
            }
        }

        VkSurfaceFormatKHR ChooseSurfaceFormat(VkPhysicalDevice device, VkSurfaceKHR surface)
        {
            std::uint32_t count = 0;
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, nullptr);
            std::vector<VkSurfaceFormatKHR> formats(count);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, formats.data());

            for (const VkFormat preferred : {VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM})
            {
                const auto found = std::ranges::find_if(formats, [preferred](const VkSurfaceFormatKHR& format)
                                                        { return format.format == preferred && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR; });
                if (found != formats.end())
                {
                    return *found;
                }
            }
            return formats.empty() ? VkSurfaceFormatKHR{VK_FORMAT_UNDEFINED, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR} : formats.front();
        }

        VkPresentModeKHR ChoosePresentMode(VkPhysicalDevice device, VkSurfaceKHR surface, bool vsync)
        {
            if (vsync)
            {
                return VK_PRESENT_MODE_FIFO_KHR;
            }

            std::uint32_t count = 0;
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &count, nullptr);
            std::vector<VkPresentModeKHR> modes(count);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &count, modes.data());

            for (const VkPresentModeKHR preferred : {VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_MAILBOX_KHR})
            {
                if (std::ranges::find(modes, preferred) != modes.end())
                {
                    return preferred;
                }
            }
            return VK_PRESENT_MODE_FIFO_KHR;
        }

        template <typename Function>
        Function LoadInstanceFunction(VkInstance instance, const char* name)
        {
            // Vulkan gives every function as PFN_vkVoidFunction, which must be converted to its real type
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            return reinterpret_cast<Function>(vkGetInstanceProcAddr(instance, name));
        }

        void TransitionImage(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout from, VkImageLayout to)
        {
            VkImageMemoryBarrier2 barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
            barrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
            barrier.srcAccessMask = from == VK_IMAGE_LAYOUT_UNDEFINED ? VK_ACCESS_2_NONE : VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
            barrier.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
            barrier.dstAccessMask = to == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR ? VK_ACCESS_2_NONE : VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
            barrier.oldLayout = from;
            barrier.newLayout = to;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = image;
            barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

            VkDependencyInfo dependency{};
            dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
            dependency.imageMemoryBarrierCount = 1;
            dependency.pImageMemoryBarriers = &barrier;
            vkCmdPipelineBarrier2(commandBuffer, &dependency);
        }
    }

    VulkanContext::VulkanContext(SDL_Window* window) : m_Window(window) {}

    VulkanContext::~VulkanContext()
    {
        if (m_Device != VK_NULL_HANDLE)
        {
            vkDeviceWaitIdle(m_Device);
            DestroySwapchainImages();
            vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr);
            for (const Frame& frame : m_Frames)
            {
                vkDestroyFence(m_Device, frame.InFlight, nullptr);
                vkDestroySemaphore(m_Device, frame.ImageAvailable, nullptr);
                vkDestroyCommandPool(m_Device, frame.CommandPool, nullptr);
            }
            vkDestroyDevice(m_Device, nullptr);
        }

        if (m_Instance != VK_NULL_HANDLE)
        {
            if (m_Surface != VK_NULL_HANDLE)
            {
                SDL_Vulkan_DestroySurface(m_Instance, m_Surface, nullptr);
            }
            if (m_Messenger != VK_NULL_HANDLE)
            {
                const auto destroyMessenger = LoadInstanceFunction<PFN_vkDestroyDebugUtilsMessengerEXT>(m_Instance, "vkDestroyDebugUtilsMessengerEXT");
                destroyMessenger(m_Instance, m_Messenger, nullptr);
            }
            vkDestroyInstance(m_Instance, nullptr);
        }
    }

    Scope<VulkanContext> VulkanContext::Create(SDL_Window* window)
    {
        Scope<VulkanContext> context(new VulkanContext(window));
        if (!context->CreateInstance() || !context->CreateSurface() || !context->SelectPhysicalDevice() || !context->CreateDevice() ||
            !context->CreateFrames() || !context->CreateSwapchain())
        {
            return nullptr;
        }
        return context;
    }

    bool VulkanContext::CreateInstance()
    {
        std::uint32_t version = 0;
        if (vkEnumerateInstanceVersion(&version) != VK_SUCCESS || version < RequiredApiVersion)
        {
            QUASAR_ENGINE_ERROR("Vulkan 1.3 is required, but the Vulkan loader only provides Vulkan {}.{}: update the graphics driver",
                                VK_API_VERSION_MAJOR(version), VK_API_VERSION_MINOR(version));
            return false;
        }

        std::uint32_t sdlExtensionCount = 0;
        const char* const* sdlExtensions = SDL_Vulkan_GetInstanceExtensions(&sdlExtensionCount);
        if (sdlExtensions == nullptr)
        {
            QUASAR_ENGINE_ERROR("Vulkan cannot draw in the window: {}", SDL_GetError());
            return false;
        }
        const std::span<const char* const> sdlExtensionList(sdlExtensions, sdlExtensionCount);
        std::vector<const char*> extensions(sdlExtensionList.begin(), sdlExtensionList.end());

        std::vector<const char*> layers;
        const bool validation = PlatformDetection::IsDebug() && HasLayer(ValidationLayer);
        if (validation)
        {
            layers.push_back(ValidationLayer);
            if (!HasExtension(GetInstanceExtensions(nullptr), VK_EXT_DEBUG_UTILS_EXTENSION_NAME) &&
                !HasExtension(GetInstanceExtensions(ValidationLayer), VK_EXT_DEBUG_UTILS_EXTENSION_NAME))
            {
                QUASAR_ENGINE_WARN("The Vulkan validation layer is installed, but its messages cannot be logged");
            }
            else
            {
                extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            }
        }
        else if (PlatformDetection::IsDebug())
        {
            QUASAR_ENGINE_INFO("The Vulkan validation layer is not installed, so the Vulkan calls are not checked");
        }

        VkApplicationInfo application{};
        application.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        application.pEngineName = "QuasarEngine";
        application.apiVersion = RequiredApiVersion;

        const VkDebugUtilsMessengerCreateInfoEXT messengerInfo = GetMessengerInfo();
        const bool logValidation = std::ranges::find(extensions, std::string_view(VK_EXT_DEBUG_UTILS_EXTENSION_NAME)) != extensions.end();

        VkInstanceCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        info.pNext = logValidation ? &messengerInfo : nullptr;
        info.pApplicationInfo = &application;
        info.enabledLayerCount = static_cast<std::uint32_t>(layers.size());
        info.ppEnabledLayerNames = layers.data();
        info.enabledExtensionCount = static_cast<std::uint32_t>(extensions.size());
        info.ppEnabledExtensionNames = extensions.data();
        if (!Succeeded(vkCreateInstance(&info, nullptr, &m_Instance), "create its instance"))
        {
            return false;
        }

        if (logValidation)
        {
            const auto createMessenger = LoadInstanceFunction<PFN_vkCreateDebugUtilsMessengerEXT>(m_Instance, "vkCreateDebugUtilsMessengerEXT");
            if (createMessenger == nullptr || !Succeeded(createMessenger(m_Instance, &messengerInfo, nullptr, &m_Messenger), "log the validation messages"))
            {
                return false;
            }
            QUASAR_ENGINE_INFO("Vulkan validation layer enabled");
        }
        return true;
    }

    bool VulkanContext::CreateSurface()
    {
        if (!SDL_Vulkan_CreateSurface(m_Window, m_Instance, nullptr, &m_Surface))
        {
            QUASAR_ENGINE_ERROR("Vulkan cannot draw in the window: {}", SDL_GetError());
            return false;
        }
        return true;
    }

    bool VulkanContext::SelectPhysicalDevice()
    {
        std::uint32_t count = 0;
        vkEnumeratePhysicalDevices(m_Instance, &count, nullptr);
        std::vector<VkPhysicalDevice> devices(count);
        vkEnumeratePhysicalDevices(m_Instance, &count, devices.data());

        int bestScore = -1;
        for (VkPhysicalDevice device : devices)
        {
            VkPhysicalDeviceProperties properties{};
            vkGetPhysicalDeviceProperties(device, &properties);

            const std::optional<std::uint32_t> queueFamily = FindQueueFamily(device, m_Surface);
            const bool usable = properties.apiVersion >= RequiredApiVersion && SupportsRequiredFeatures(device) &&
                                HasExtension(GetDeviceExtensions(device), VK_KHR_SWAPCHAIN_EXTENSION_NAME) && queueFamily.has_value();
            if (usable && GetDeviceScore(properties) > bestScore)
            {
                bestScore = GetDeviceScore(properties);
                m_PhysicalDevice = device;
                m_QueueFamily = *queueFamily;
            }
        }

        if (m_PhysicalDevice == VK_NULL_HANDLE)
        {
            QUASAR_ENGINE_ERROR("No GPU supports Vulkan 1.3 and can draw in the window: update the graphics driver");
            return false;
        }

        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(m_PhysicalDevice, &properties);
        QUASAR_ENGINE_INFO("Vulkan device: {}", properties.deviceName);
        QUASAR_ENGINE_INFO("Vulkan version: {}.{}.{}", VK_API_VERSION_MAJOR(properties.apiVersion), VK_API_VERSION_MINOR(properties.apiVersion),
                           VK_API_VERSION_PATCH(properties.apiVersion));
        return true;
    }

    bool VulkanContext::CreateDevice()
    {
        const float priority = 1.0f;
        VkDeviceQueueCreateInfo queue{};
        queue.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue.queueFamilyIndex = m_QueueFamily;
        queue.queueCount = 1;
        queue.pQueuePriorities = &priority;

        VkPhysicalDeviceVulkan13Features features13{};
        features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
        features13.dynamicRendering = VK_TRUE;
        features13.synchronization2 = VK_TRUE;

        const std::array<const char*, 1> extensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME};

        VkDeviceCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        info.pNext = &features13;
        info.queueCreateInfoCount = 1;
        info.pQueueCreateInfos = &queue;
        info.enabledExtensionCount = static_cast<std::uint32_t>(extensions.size());
        info.ppEnabledExtensionNames = extensions.data();
        if (!Succeeded(vkCreateDevice(m_PhysicalDevice, &info, nullptr, &m_Device), "create its device"))
        {
            return false;
        }

        vkGetDeviceQueue(m_Device, m_QueueFamily, 0, &m_Queue);
        return true;
    }

    bool VulkanContext::CreateFrames()
    {
        for (Frame& frame : m_Frames)
        {
            VkCommandPoolCreateInfo poolInfo{};
            poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
            poolInfo.queueFamilyIndex = m_QueueFamily;
            if (!Succeeded(vkCreateCommandPool(m_Device, &poolInfo, nullptr, &frame.CommandPool), "create a command pool"))
            {
                return false;
            }

            VkCommandBufferAllocateInfo bufferInfo{};
            bufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            bufferInfo.commandPool = frame.CommandPool;
            bufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            bufferInfo.commandBufferCount = 1;
            if (!Succeeded(vkAllocateCommandBuffers(m_Device, &bufferInfo, &frame.CommandBuffer), "allocate a command buffer"))
            {
                return false;
            }

            VkSemaphoreCreateInfo semaphoreInfo{};
            semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            if (!Succeeded(vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &frame.ImageAvailable), "create a semaphore"))
            {
                return false;
            }

            VkFenceCreateInfo fenceInfo{};
            fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
            if (!Succeeded(vkCreateFence(m_Device, &fenceInfo, nullptr, &frame.InFlight), "create a fence"))
            {
                return false;
            }
        }
        return true;
    }

    bool VulkanContext::CreateSwapchain()
    {
        VkSurfaceCapabilitiesKHR capabilities{};
        if (!Succeeded(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDevice, m_Surface, &capabilities), "read the surface of the window"))
        {
            return false;
        }

        VkExtent2D extent = capabilities.currentExtent;
        if (extent.width == std::numeric_limits<std::uint32_t>::max())
        {
            int width = 0;
            int height = 0;
            SDL_GetWindowSizeInPixels(m_Window, &width, &height);
            extent.width = std::clamp(static_cast<std::uint32_t>(width), capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            extent.height = std::clamp(static_cast<std::uint32_t>(height), capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
        }
        if (extent.width == 0 || extent.height == 0)
        {
            m_SwapchainOutdated = true;
            return true;
        }

        if (m_ImageFormat == VK_FORMAT_UNDEFINED)
        {
            const VkSurfaceFormatKHR format = ChooseSurfaceFormat(m_PhysicalDevice, m_Surface);
            m_ImageFormat = format.format;
            m_ColorSpace = format.colorSpace;
        }

        std::uint32_t imageCount = std::max(capabilities.minImageCount + 1, PreferredImageCount);
        if (capabilities.maxImageCount > 0)
        {
            imageCount = std::min(imageCount, capabilities.maxImageCount);
        }

        VkSwapchainKHR oldSwapchain = m_Swapchain;
        VkSwapchainCreateInfoKHR info{};
        info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        info.surface = m_Surface;
        info.minImageCount = imageCount;
        info.imageFormat = m_ImageFormat;
        info.imageColorSpace = m_ColorSpace;
        info.imageExtent = extent;
        info.imageArrayLayers = 1;
        info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        info.preTransform = capabilities.currentTransform;
        info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        info.presentMode = ChoosePresentMode(m_PhysicalDevice, m_Surface, m_VSync);
        info.clipped = VK_TRUE;
        info.oldSwapchain = oldSwapchain;

        VkSwapchainKHR swapchain = VK_NULL_HANDLE;
        if (!Succeeded(vkCreateSwapchainKHR(m_Device, &info, nullptr, &swapchain), "create the swapchain"))
        {
            return false;
        }

        DestroySwapchainImages();
        vkDestroySwapchainKHR(m_Device, oldSwapchain, nullptr);
        m_Swapchain = swapchain;
        m_ImageExtent = extent;
        m_SwapchainOutdated = false;

        std::uint32_t count = 0;
        vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &count, nullptr);
        m_Images.resize(count);
        vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &count, m_Images.data());

        for (VkImage image : m_Images)
        {
            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = image;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = m_ImageFormat;
            viewInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
            VkImageView view = VK_NULL_HANDLE;
            if (!Succeeded(vkCreateImageView(m_Device, &viewInfo, nullptr, &view), "create a view of a swapchain image"))
            {
                return false;
            }
            m_ImageViews.push_back(view);

            VkSemaphoreCreateInfo semaphoreInfo{};
            semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            VkSemaphore semaphore = VK_NULL_HANDLE;
            if (!Succeeded(vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &semaphore), "create a semaphore"))
            {
                return false;
            }
            m_RenderFinished.push_back(semaphore);
        }
        return true;
    }

    void VulkanContext::DestroySwapchainImages()
    {
        for (VkImageView view : m_ImageViews)
        {
            vkDestroyImageView(m_Device, view, nullptr);
        }
        for (VkSemaphore semaphore : m_RenderFinished)
        {
            vkDestroySemaphore(m_Device, semaphore, nullptr);
        }
        m_ImageViews.clear();
        m_RenderFinished.clear();
        m_Images.clear();
    }

    bool VulkanContext::BeginFrame()
    {
        int width = 0;
        int height = 0;
        SDL_GetWindowSizeInPixels(m_Window, &width, &height);
        if (width <= 0 || height <= 0)
        {
            return false;
        }

        if (m_SwapchainOutdated || m_Swapchain == VK_NULL_HANDLE || static_cast<std::uint32_t>(width) != m_ImageExtent.width ||
            static_cast<std::uint32_t>(height) != m_ImageExtent.height)
        {
            vkDeviceWaitIdle(m_Device);
            if (!CreateSwapchain() || m_SwapchainOutdated)
            {
                return false;
            }
        }

        const Frame& frame = m_Frames.at(m_FrameIndex);
        vkWaitForFences(m_Device, 1, &frame.InFlight, VK_TRUE, std::numeric_limits<std::uint64_t>::max());

        const VkResult acquired =
            vkAcquireNextImageKHR(m_Device, m_Swapchain, std::numeric_limits<std::uint64_t>::max(), frame.ImageAvailable, VK_NULL_HANDLE, &m_ImageIndex);
        if (acquired == VK_ERROR_OUT_OF_DATE_KHR)
        {
            m_SwapchainOutdated = true;
            return false;
        }
        if (acquired != VK_SUBOPTIMAL_KHR && !Succeeded(acquired, "take the next image of the window"))
        {
            return false;
        }

        vkResetFences(m_Device, 1, &frame.InFlight);
        vkResetCommandPool(m_Device, frame.CommandPool, 0);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(frame.CommandBuffer, &beginInfo);

        TransitionImage(frame.CommandBuffer, m_Images.at(m_ImageIndex), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        VkRenderingAttachmentInfo colorAttachment{};
        colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachment.imageView = m_ImageViews.at(m_ImageIndex);
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.clearValue = VkClearValue{.color = VkClearColorValue{.float32 = {ClearColor[0], ClearColor[1], ClearColor[2], ClearColor[3]}}};

        VkRenderingInfo renderingInfo{};
        renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        renderingInfo.renderArea = {{0, 0}, m_ImageExtent};
        renderingInfo.layerCount = 1;
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachments = &colorAttachment;
        vkCmdBeginRendering(frame.CommandBuffer, &renderingInfo);

        m_FrameStarted = true;
        return true;
    }

    void VulkanContext::SwapBuffers()
    {
        if (!m_FrameStarted)
        {
            return;
        }
        m_FrameStarted = false;

        const Frame& frame = m_Frames.at(m_FrameIndex);
        vkCmdEndRendering(frame.CommandBuffer);
        TransitionImage(frame.CommandBuffer, m_Images.at(m_ImageIndex), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
        vkEndCommandBuffer(frame.CommandBuffer);

        VkSemaphoreSubmitInfo waitInfo{};
        waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        waitInfo.semaphore = frame.ImageAvailable;
        waitInfo.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

        VkSemaphoreSubmitInfo signalInfo{};
        signalInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        signalInfo.semaphore = m_RenderFinished.at(m_ImageIndex);
        signalInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

        VkCommandBufferSubmitInfo commandInfo{};
        commandInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
        commandInfo.commandBuffer = frame.CommandBuffer;

        VkSubmitInfo2 submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
        submitInfo.waitSemaphoreInfoCount = 1;
        submitInfo.pWaitSemaphoreInfos = &waitInfo;
        submitInfo.commandBufferInfoCount = 1;
        submitInfo.pCommandBufferInfos = &commandInfo;
        submitInfo.signalSemaphoreInfoCount = 1;
        submitInfo.pSignalSemaphoreInfos = &signalInfo;
        if (!Succeeded(vkQueueSubmit2(m_Queue, 1, &submitInfo, frame.InFlight), "submit the commands of the frame"))
        {
            return;
        }

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &m_RenderFinished.at(m_ImageIndex);
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &m_Swapchain;
        presentInfo.pImageIndices = &m_ImageIndex;
        const VkResult presented = vkQueuePresentKHR(m_Queue, &presentInfo);
        if (presented == VK_ERROR_OUT_OF_DATE_KHR || presented == VK_SUBOPTIMAL_KHR)
        {
            m_SwapchainOutdated = true;
        }
        else
        {
            Succeeded(presented, "present the image of the frame");
        }

        m_FrameIndex = (m_FrameIndex + 1) % FramesInFlight;
    }

    void VulkanContext::SetVSync(bool enabled)
    {
        if (enabled != m_VSync)
        {
            m_VSync = enabled;
            m_SwapchainOutdated = true;
        }
    }

    VkCommandBuffer VulkanContext::GetCommandBuffer() const
    {
        return m_FrameStarted ? m_Frames.at(m_FrameIndex).CommandBuffer : VK_NULL_HANDLE;
    }
}

#pragma once

/// \file
/// \brief The Vulkan 1.3 context of a window.
/// \author Kurogenshi
/// \date 2026-11-12

#include <quasar/core/Base.h>
#include <quasar/renderer/GraphicsAPI.h>
#include <quasar/renderer/GraphicsContext.h>

#include <vulkan/vulkan.h>

#include <array>
#include <cstdint>
#include <vector>

struct SDL_Window;

namespace Quasar
{
    /// @brief Vulkan 1.3 context of a window, created with SDL: the instance, the device, the swapchain and the command
    /// buffers of the frames.
    ///
    /// A window created with GraphicsAPI::Vulkan owns one, which Window::GetGraphicsContext() returns. The driver must
    /// support Vulkan 1.3, which gives dynamic rendering and synchronization2 without extensions. When several GPUs
    /// can draw in the window, a discrete GPU is preferred.
    ///
    /// Each frame, BeginFrame() takes an image of the swapchain, starts recording the command buffer of the frame and
    /// begins dynamic rendering into the image, cleared with ClearColor. The code that draws records its commands into
    /// GetCommandBuffer(). SwapBuffers() then ends the rendering, submits the command buffer and presents the image. The
    /// CPU records up to FramesInFlight frames while the GPU draws the previous ones.
    ///
    /// The swapchain is created again when the size of the window or VSync changes. On Android, the surface is created
    /// again when the app comes back to the foreground, since Android destroys the window of an app in the background.
    /// In Debug, the Khronos validation layer is enabled when it is installed, and its warnings and errors are logged as
    /// engine warnings and errors.
    class VulkanContext final : public GraphicsContext
    {
    public:
        /// @brief Number of frames that the CPU can record while the GPU draws the previous ones.
        static constexpr std::uint32_t FramesInFlight = 2;

        /// @brief Creates the context of a window: the instance, the surface, the device and the swapchain.
        /// @param window The SDL window, created with SDL_WINDOW_VULKAN.
        /// @return The context, or nullptr if no GPU supports Vulkan 1.3 and can draw in the window. The reason is
        /// logged as an engine error.
        static Scope<VulkanContext> Create(SDL_Window* window);

        ~VulkanContext() override;

        VulkanContext(const VulkanContext&) = delete;
        VulkanContext& operator=(const VulkanContext&) = delete;
        VulkanContext(VulkanContext&&) = delete;
        VulkanContext& operator=(VulkanContext&&) = delete;

        /// @brief Returns GraphicsAPI::Vulkan.
        GraphicsAPI GetAPI() const override { return GraphicsAPI::Vulkan; }

        /// @brief Takes the next image of the swapchain and begins the rendering into it.
        /// @return False while the window has no image, for example while its size is 0, or when the swapchain has to be
        /// created again. Nothing must be drawn then.
        bool BeginFrame() override;

        /// @brief Ends the rendering of the frame, submits its command buffer and presents its image. Does nothing when
        /// no frame was begun.
        void SwapBuffers() override;

        /// @brief Enables or disables vertical synchronization. The swapchain is created again at the next frame.
        ///
        /// Without VSync, the mailbox mode is used when the driver supports it, otherwise the immediate mode.
        void SetVSync(bool enabled) override;

        /// @brief Returns the Vulkan instance.
        VkInstance GetInstance() const { return m_Instance; }

        /// @brief Returns the GPU that draws in the window.
        VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }

        /// @brief Returns the logical device.
        VkDevice GetDevice() const { return m_Device; }

        /// @brief Returns the index of the queue family used for drawing and presenting.
        std::uint32_t GetQueueFamily() const { return m_QueueFamily; }

        /// @brief Returns the queue used for drawing and presenting.
        VkQueue GetQueue() const { return m_Queue; }

        /// @brief Returns the format of the images of the swapchain.
        VkFormat GetImageFormat() const { return m_ImageFormat; }

        /// @brief Returns the size of the images of the swapchain, in pixels. It follows the size of the window.
        VkExtent2D GetImageExtent() const { return m_ImageExtent; }

        /// @brief Returns the number of images in the swapchain.
        std::uint32_t GetImageCount() const { return static_cast<std::uint32_t>(m_Images.size()); }

        /// @brief Returns the command buffer of the current frame, between BeginFrame() and SwapBuffers(), or
        /// VK_NULL_HANDLE outside of a frame.
        VkCommandBuffer GetCommandBuffer() const;

    private:
        struct Frame
        {
            VkCommandPool CommandPool = VK_NULL_HANDLE;
            VkCommandBuffer CommandBuffer = VK_NULL_HANDLE;
            VkSemaphore ImageAvailable = VK_NULL_HANDLE;
            VkFence InFlight = VK_NULL_HANDLE;
        };

        explicit VulkanContext(SDL_Window* window);

        bool CreateInstance();
        bool CreateSurface();
        bool RecreateSurface();
        bool SelectPhysicalDevice();
        bool CreateDevice();
        bool CreateFrames();
        bool CreateSwapchain();
        void DestroySwapchainImages();

        SDL_Window* m_Window = nullptr;
        VkInstance m_Instance = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT m_Messenger = VK_NULL_HANDLE;
        VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
        void* m_SurfaceWindow = nullptr;
        VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
        VkDevice m_Device = VK_NULL_HANDLE;
        std::uint32_t m_QueueFamily = 0;
        VkQueue m_Queue = VK_NULL_HANDLE;

        VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
        VkFormat m_ImageFormat = VK_FORMAT_UNDEFINED;
        VkColorSpaceKHR m_ColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        VkExtent2D m_ImageExtent{0, 0};
        std::vector<VkImage> m_Images;
        std::vector<VkImageView> m_ImageViews;
        std::vector<VkSemaphore> m_RenderFinished;

        std::array<Frame, FramesInFlight> m_Frames{};
        std::uint32_t m_FrameIndex = 0;
        std::uint32_t m_ImageIndex = 0;
        bool m_FrameStarted = false;
        bool m_SwapchainOutdated = false;
        bool m_VSync = true;
    };
}

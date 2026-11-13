#include "helpers/LogHelpers.h"
#include "helpers/NullWindow.h"

#include <quasar/core/Window.h>
#include <quasar/log/MemoryLogSink.h>
#include <quasar/platform/vulkan/VulkanContext.h>
#include <quasar/renderer/GraphicsAPI.h>

#include <doctest/doctest.h>

#include <SDL3/SDL_video.h>
#include <vulkan/vulkan.h>

#include <cstdint>

namespace
{
    Quasar::VulkanContext* GetVulkanContext(const Quasar::Window& window)
    {
        return dynamic_cast<Quasar::VulkanContext*>(window.GetGraphicsContext());
    }
}

TEST_CASE("A window gets a Vulkan 1.3 device that can present to it")
{
    const TestHelpers::ScopedLog log;

    const Quasar::Scope<Quasar::Window> window = TestHelpers::CreateWindowWithContext({.Graphics = Quasar::GraphicsAPI::Vulkan});

    REQUIRE(window != nullptr);
    CHECK(window->GetGraphicsAPI() == Quasar::GraphicsAPI::Vulkan);
    const Quasar::VulkanContext* context = GetVulkanContext(*window);
    REQUIRE(context != nullptr);
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(context->GetPhysicalDevice(), &properties);
    CHECK(properties.apiVersion >= VK_API_VERSION_1_3);
    CHECK(context->GetDevice() != VK_NULL_HANDLE);
    CHECK(context->GetQueue() != VK_NULL_HANDLE);
    CHECK(context->GetImageCount() >= Quasar::VulkanContext::FramesInFlight);
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
}

TEST_CASE("A Vulkan frame records commands only between its beginning and its presentation")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Window> window = TestHelpers::CreateWindowWithContext({.Graphics = Quasar::GraphicsAPI::Vulkan});
    REQUIRE(window != nullptr);
    Quasar::VulkanContext* context = GetVulkanContext(*window);
    REQUIRE(context != nullptr);

    CHECK(context->GetCommandBuffer() == VK_NULL_HANDLE);
    for (int frame = 0; frame < 3; ++frame)
    {
        REQUIRE(context->BeginFrame());
        CHECK(context->GetCommandBuffer() != VK_NULL_HANDLE);
        window->Update();
        CHECK(context->GetCommandBuffer() == VK_NULL_HANDLE);
    }

    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
}

TEST_CASE("The Vulkan images follow the size of the window and the VSync setting")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Window> window = TestHelpers::CreateWindowWithContext({.Graphics = Quasar::GraphicsAPI::Vulkan});
    REQUIRE(window != nullptr);
    Quasar::VulkanContext* context = GetVulkanContext(*window);
    REQUIRE(context != nullptr);

    REQUIRE(SDL_SetWindowSize(window->GetNativeWindow(), 320, 240));
    SDL_SyncWindow(window->GetNativeWindow());
    window->SetVSync(false);
    REQUIRE(context->BeginFrame());
    window->Update();

    int width = 0;
    int height = 0;
    SDL_GetWindowSizeInPixels(window->GetNativeWindow(), &width, &height);
    CHECK(context->GetImageExtent().width == static_cast<std::uint32_t>(width));
    CHECK(context->GetImageExtent().height == static_cast<std::uint32_t>(height));
    CHECK_FALSE(window->IsVSync());
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
}

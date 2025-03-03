#pragma once

#include <QuasarEngine/EngineFactory.h>
#include <QuasarEngine/Renderer/IRenderer.h>

#include <iostream>

class VulkanRenderer : public IRenderer
{
public:
    VulkanRenderer() {}
    ~VulkanRenderer() override
    {
        Shutdown();
    }

    void Initialize() override
    {
		std::cout << "VulkanRenderer::Initialize()" << std::endl;
    }

    void Render() override
    {
        std::cout << "VulkanRenderer::Render()" << std::endl;
    }

    void Shutdown() override
    {
        std::cout << "VulkanRenderer::Shutdown()" << std::endl;
    }

    static void Register()
    {
        EngineFactory::Instance().RegisterRenderer(RendererAPI::Vulkan, [] {
            return std::make_unique<VulkanRenderer>();
        });
    }
};
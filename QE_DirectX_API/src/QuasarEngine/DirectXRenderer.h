#pragma once

#include <QuasarEngine/EngineFactory.h>
#include <QuasarEngine/Renderer/IRenderer.h>

#include <iostream>

class DirectXRenderer : public IRenderer
{
public:
    DirectXRenderer() {}
    ~DirectXRenderer() override
    {
        Shutdown();
    }

    void Initialize() override
    {
        std::cout << "DirectXRenderer::Initialize()" << std::endl;
    }

    void Render() override
    {
        std::cout << "DirectXRenderer::Render()" << std::endl;
    }

    void Shutdown() override
    {
        std::cout << "DirectXRenderer::Shutdown()" << std::endl;
    }

    static void Register()
    {
        EngineFactory::Instance().RegisterRenderer(RendererAPI::DirectX, [] {
            return std::make_unique<DirectXRenderer>();
        });
    }
};
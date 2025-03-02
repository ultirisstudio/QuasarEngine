#pragma once

#include <QuasarEngine/EngineFactory.h>
#include <QuasarEngine/Renderer/IRenderer.h>

#include <iostream>

class OpenGLRenderer : public IRenderer
{
public:
    OpenGLRenderer()
    {
        std::cout << "OpenGLRenderer::OpenGLRenderer()" << std::endl;
    }

    ~OpenGLRenderer() override
    {
		std::cout << "OpenGLRenderer::~OpenGLRenderer()" << std::endl;
    }

    void Initialize() override
    {
        std::cout << "OpenGLRenderer::Initialize()" << std::endl;
    }

    void Render() override
    {
        std::cout << "OpenGLRenderer::Render()" << std::endl;
    }

    void Shutdown() override
    {
        std::cout << "OpenGLRenderer::Shutdown()" << std::endl;
    }

    static void Register()
    {
        EngineFactory::Instance().RegisterRenderer(RendererAPI::OpenGL, [] {
            return std::make_unique<OpenGLRenderer>();
        });
    }
};
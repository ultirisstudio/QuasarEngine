#pragma once

#include <QuasarEngine/Renderer/RenderContext.h>
#include <QuasarEngine/Renderer/RenderObject.h>

namespace QuasarEngine
{
    class IRenderTechnique
    {
    public:
        virtual ~IRenderTechnique() = default;

        virtual void Begin(RenderContext& ctx) = 0;
        virtual void Submit(RenderContext& ctx, RenderObject& obj) = 0;
        virtual void End() = 0;
    };
}
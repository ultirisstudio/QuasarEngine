#include <quasar/renderer/FullscreenPass.h>

#include <quasar/log/Log.h>

#include <utility>

namespace Quasar
{
    FullscreenPass::FullscreenPass(GraphicsContext& context, Scope<VertexArray> emptyVertexArray)
        : m_Context(&context), m_EmptyVertexArray(std::move(emptyVertexArray))
    {
    }

    Scope<FullscreenPass> FullscreenPass::Create(GraphicsContext& context)
    {
        Scope<VertexArray> emptyVertexArray = context.CreateVertexArray();
        if (emptyVertexArray == nullptr)
        {
            QUASAR_ENGINE_ERROR("Cannot create a fullscreen pass: its vertex array cannot be created");
            return nullptr;
        }
        return Scope<FullscreenPass>(new FullscreenPass(context, std::move(emptyVertexArray)));
    }

    void FullscreenPass::Draw(Pipeline& pipeline, Framebuffer* target)
    {
        if (target != nullptr)
        {
            target->Bind();
        }
        else
        {
            m_Context->BindWindow();
        }

        pipeline.Bind();
        m_Context->Draw(*m_EmptyVertexArray, 3);

        if (target != nullptr)
        {
            target->Unbind();
            m_Context->BindWindow();
        }
    }
}

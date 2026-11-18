#include <quasar/renderer/Framebuffer.h>

#include <quasar/log/Log.h>

#include <algorithm>
#include <utility>

namespace Quasar
{
    Framebuffer::Framebuffer(FramebufferSpecification specification) : m_Specification(std::move(specification)) {}

    bool Framebuffer::IsValid(const FramebufferSpecification& specification)
    {
        if (specification.Width == 0 || specification.Height == 0)
        {
            QUASAR_ENGINE_ERROR("Cannot create a framebuffer of {}x{} pixels", specification.Width, specification.Height);
            return false;
        }
        if (specification.Attachments.empty())
        {
            QUASAR_ENGINE_ERROR("Cannot create a framebuffer without attachments");
            return false;
        }
        if (std::ranges::count(specification.Attachments, FramebufferFormat::Depth24Stencil8) > 1)
        {
            QUASAR_ENGINE_ERROR("Cannot create a framebuffer with more than one depth attachment");
            return false;
        }
        if (specification.Samples == 0)
        {
            QUASAR_ENGINE_ERROR("Cannot create a framebuffer with 0 samples per pixel");
            return false;
        }
        return true;
    }

    void Framebuffer::SetSize(std::uint32_t width, std::uint32_t height)
    {
        m_Specification.Width = width;
        m_Specification.Height = height;
    }
}

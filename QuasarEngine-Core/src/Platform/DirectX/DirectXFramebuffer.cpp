#include "qepch.h"

#include "DirectXFramebuffer.h"

namespace QuasarEngine
{
    DirectXFramebuffer::DirectXFramebuffer(const FramebufferSpecification& spec) : Framebuffer(spec), m_ID(0)
    {
        
    }

    DirectXFramebuffer::~DirectXFramebuffer()
    {
        
    }

    void* DirectXFramebuffer::GetColorAttachment(uint32_t index) const
    {
        if (index >= m_ColorAttachments.size()) return nullptr;
        return reinterpret_cast<void*>(static_cast<uintptr_t>(m_ColorAttachments[index]));
    }

    void* DirectXFramebuffer::GetDepthAttachment() const
    {
        return reinterpret_cast<void*>(static_cast<uintptr_t>(m_DepthAttachment));
    }

    int DirectXFramebuffer::ReadPixel(uint32_t attachmentIndex, int x, int y)
    {
        if (attachmentIndex >= m_ColorAttachments.size())
            return -1;

        int pixelData = 0;
        return pixelData;
    }

    void DirectXFramebuffer::ClearAttachment(uint32_t attachmentIndex, int value)
    {
        
    }

    void DirectXFramebuffer::Resize(uint32_t width, uint32_t height)
    {
        
    }

    void DirectXFramebuffer::Invalidate()
    {
        
    }

    void DirectXFramebuffer::Resolve()
    {
        
    }

    void DirectXFramebuffer::Bind() const
    {
        
    }

    void DirectXFramebuffer::Unbind() const
    {
        
    }

    void DirectXFramebuffer::BindColorAttachment(uint32_t index) const
    {
        
    }
}
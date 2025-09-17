#pragma once

#include <vector>
#include <memory>

namespace QuasarEngine
{
    enum class FramebufferTextureFormat
    {
		None = 0,

		RGBA8,
		RED_INTEGER,

		DEPTH24STENCIL8,
		Depth = DEPTH24STENCIL8
	};

    struct FramebufferTextureSpecification
    {
        FramebufferTextureSpecification() = default;
        FramebufferTextureSpecification(FramebufferTextureFormat format) : TextureFormat(format) {}
        FramebufferTextureFormat TextureFormat = FramebufferTextureFormat::None;
    };

    struct FramebufferAttachmentSpecification
    {
        FramebufferAttachmentSpecification() = default;
        FramebufferAttachmentSpecification(std::initializer_list<FramebufferTextureSpecification> attachments) : Attachments(attachments) {}
        std::vector<FramebufferTextureSpecification> Attachments;
    };

    struct FramebufferSpecification
    {
        uint32_t Width = 1280, Height = 720;
        FramebufferAttachmentSpecification Attachments;
        uint32_t Samples = 1;
    };

    class Framebuffer
    {
    public:
        Framebuffer(const FramebufferSpecification& specification);
        virtual ~Framebuffer() = default;

        virtual void Bind() const = 0;
        virtual void Unbind() const = 0;

        virtual void Resize(uint32_t width, uint32_t height) = 0;

        virtual void Invalidate() = 0;

        virtual void Resolve() = 0;

        virtual int ReadPixel(uint32_t attachmentIndex, int x, int y) = 0;
        virtual void ClearAttachment(uint32_t attachmentIndex, int value) = 0;

        virtual void BindColorAttachment(uint32_t index = 0) const = 0;

        virtual void* GetColorAttachment(uint32_t index) const = 0;
        virtual void* GetDepthAttachment() const = 0;

		const FramebufferSpecification& GetSpecification() const { return m_Specification; }

        static std::shared_ptr<Framebuffer> Create(const FramebufferSpecification& spec);

    protected:
        FramebufferSpecification m_Specification;
    };
}

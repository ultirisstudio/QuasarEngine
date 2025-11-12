#pragma once

#include <vector>
#include <memory>

namespace QuasarEngine
{
    enum class FramebufferTextureFormat {
        None = 0,
        RGBA8,
        RED_INTEGER,
        DEPTH24,
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

    struct FramebufferSpecification {
        uint32_t Width = 1280, Height = 720;
        FramebufferAttachmentSpecification Attachments;
        uint32_t Samples = 1;
        bool DepthAsTexture = false;
    };

    class Texture;

    struct AttachmentRef {
        std::shared_ptr<Texture> texture;
        uint32_t mip = 0;
        uint32_t layer = 0;
    };

    enum class ClearFlags : uint8_t {
        None = 0,
        Color = 1 << 0,
        Depth = 1 << 1,
        Stencil = 1 << 2,
        All = (1 << 0) | (1 << 1) | (1 << 2)
    };

    inline constexpr ClearFlags operator|(ClearFlags a, ClearFlags b) {
        return static_cast<ClearFlags>(
            static_cast<uint8_t>(a) | static_cast<uint8_t>(b)
            );
    }
    inline constexpr ClearFlags operator&(ClearFlags a, ClearFlags b) {
        return static_cast<ClearFlags>(
            static_cast<uint8_t>(a) & static_cast<uint8_t>(b)
            );
    }
    inline constexpr ClearFlags& operator|=(ClearFlags& a, ClearFlags b) {
        a = a | b; return a;
    }

    using FramebufferHandle = std::uintptr_t;

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

        virtual void ClearAttachment(uint32_t attachmentIndex, float r, float g, float b, float a) = 0;
        virtual void ClearColor(float r, float g, float b, float a) = 0;
        virtual void ClearDepth(float d = 1.0f) = 0;
        virtual void Clear(ClearFlags flags = ClearFlags::All) = 0;

        virtual void BindColorAttachment(uint32_t index = 0) const = 0;

        virtual void SetColorAttachment(uint32_t index, const AttachmentRef& ref) = 0;

        virtual void* GetColorAttachment(uint32_t index) const = 0;
        virtual void* GetDepthAttachment() const = 0;

        virtual std::shared_ptr<Texture> GetColorAttachmentTexture(uint32_t index) const = 0;
        virtual std::shared_ptr<Texture> GetDepthAttachmentTexture() const = 0;

		const FramebufferSpecification& GetSpecification() const { return m_Specification; }

        static std::shared_ptr<Framebuffer> Create(const FramebufferSpecification& spec);

    protected:
        FramebufferSpecification m_Specification;
    };
}

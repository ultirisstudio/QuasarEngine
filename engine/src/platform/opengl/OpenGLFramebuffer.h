#pragma once

#include <quasar/core/Base.h>
#include <quasar/renderer/Framebuffer.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace Quasar
{
    class OpenGLFramebuffer final : public Framebuffer
    {
    public:
        static Scope<OpenGLFramebuffer> Create(const FramebufferSpecification& specification);

        ~OpenGLFramebuffer() override;

        OpenGLFramebuffer(const OpenGLFramebuffer&) = delete;
        OpenGLFramebuffer& operator=(const OpenGLFramebuffer&) = delete;
        OpenGLFramebuffer(OpenGLFramebuffer&&) = delete;
        OpenGLFramebuffer& operator=(OpenGLFramebuffer&&) = delete;

        void Bind() override;
        void Unbind() override;
        void Resize(std::uint32_t width, std::uint32_t height) override;

        void ClearColor(std::uint32_t attachment, glm::vec4 color) override;
        void ClearInteger(std::uint32_t attachment, int value) override;
        void ClearDepthStencil(float depth, std::uint8_t stencil) override;

        std::optional<int> ReadInteger(std::uint32_t attachment, std::uint32_t x, std::uint32_t y) override;
        ImTextureID GetImGuiTexture(std::uint32_t attachment) const override;

    private:
        explicit OpenGLFramebuffer(const FramebufferSpecification& specification);

        bool CreateTargets();
        void DestroyTargets();
        void Resolve();
        bool IsMultisampled() const;
        std::optional<std::size_t> FindColor(std::uint32_t attachment, bool integer, const char* action) const;
        std::uint32_t GetReadableTexture(std::size_t index) const;

        std::vector<FramebufferFormat> m_ColorFormats;
        bool m_HasDepthStencil = false;

        std::uint32_t m_Framebuffer = 0;
        std::vector<std::uint32_t> m_ColorTextures;
        std::uint32_t m_DepthStencil = 0;
        std::uint32_t m_ResolveFramebuffer = 0;
        std::vector<std::uint32_t> m_ResolvedTextures;
    };
}

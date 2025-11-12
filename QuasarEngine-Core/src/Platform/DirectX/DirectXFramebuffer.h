#pragma once

#include <QuasarEngine/Renderer/Framebuffer.h>
#include <wrl/client.h>
#include <d3d11.h>
#include <vector>

namespace QuasarEngine
{
    class Texture;

    class DirectXFramebuffer : public Framebuffer
    {
    public:
        DirectXFramebuffer(const FramebufferSpecification& spec);
        ~DirectXFramebuffer() override;

        DirectXFramebuffer(const DirectXFramebuffer&) = delete;
        DirectXFramebuffer& operator=(const DirectXFramebuffer&) = delete;

        void* GetColorAttachment(uint32_t index) const override;
        void* GetDepthAttachment() const override; 

        int ReadPixel(uint32_t attachmentIndex, int x, int y) override;

        void ClearAttachment(uint32_t attachmentIndex, float r, float g, float b, float a) override;
        void ClearColor(float r, float g, float b, float a) override {}
        void ClearDepth(float d = 1.0f) override {}
        void Clear(ClearFlags flags = ClearFlags::All) override {}

        void Resize(uint32_t width, uint32_t height) override;
        void Invalidate() override;
        void Resolve() override;

        void Bind() const override;
        void Unbind() const override;

        void BindColorAttachment(uint32_t index = 0) const override;

        void SetColorAttachment(uint32_t index, const AttachmentRef& ref) override {}

        std::shared_ptr<Texture> GetColorAttachmentTexture(uint32_t index) const override { return nullptr; }
        std::shared_ptr<Texture> GetDepthAttachmentTexture() const override { return nullptr; }

    private:
        struct ColorAttachmentDX
        {
            Microsoft::WRL::ComPtr<ID3D11Texture2D>            texture;
            Microsoft::WRL::ComPtr<ID3D11RenderTargetView>     rtv;
            Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>   srv;

            Microsoft::WRL::ComPtr<ID3D11Texture2D>            resolveTexture;
            Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>   resolveSRV;
        };

        struct DepthAttachmentDX
        {
            Microsoft::WRL::ComPtr<ID3D11Texture2D>          texture;
            Microsoft::WRL::ComPtr<ID3D11DepthStencilView>   dsv;
        };

    private:
        DXGI_FORMAT ToDXColorFormat(FramebufferTextureFormat fmt) const;
        bool        IsDepthFormat(FramebufferTextureFormat fmt) const;

        void        ReleaseResources();

    private:
        FramebufferSpecification m_Spec;
        std::vector<FramebufferTextureSpecification> m_ColorSpecs;
        FramebufferTextureSpecification               m_DepthSpec;

        std::vector<ColorAttachmentDX> m_Color;
        DepthAttachmentDX              m_Depth;

        mutable D3D11_VIEWPORT m_Viewport{};
    };
}

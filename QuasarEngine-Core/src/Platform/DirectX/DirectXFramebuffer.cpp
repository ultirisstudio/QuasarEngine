#include "qepch.h"
#include "DirectXFramebuffer.h"

#include "DirectXContext.h"
#include <algorithm>
#include <cstring>

#include <QuasarEngine/Core/Logger.h>

namespace QuasarEngine
{
    static bool IsDepthFmt(FramebufferTextureFormat f)
    {
        return f == FramebufferTextureFormat::DEPTH24STENCIL8;
    }

    DXGI_FORMAT DirectXFramebuffer::ToDXColorFormat(FramebufferTextureFormat fmt) const
    {
        switch (fmt)
        {
        case FramebufferTextureFormat::RGBA8:      return DXGI_FORMAT_R8G8B8A8_UNORM;
        default:                                   return DXGI_FORMAT_UNKNOWN;
        }
    }

    bool DirectXFramebuffer::IsDepthFormat(FramebufferTextureFormat fmt) const
    {
        return IsDepthFmt(fmt);
    }

    DirectXFramebuffer::DirectXFramebuffer(const FramebufferSpecification& spec)
        : Framebuffer(spec), m_Spec(spec)
    {
        for (auto& at : spec.Attachments.Attachments)
        {
            if (!IsDepthFormat(at.TextureFormat))
                m_ColorSpecs.push_back(at);
            else
                m_DepthSpec = at;
        }

        Invalidate();
    }

    DirectXFramebuffer::~DirectXFramebuffer()
    {
        ReleaseResources();
    }

    void DirectXFramebuffer::ReleaseResources()
    {
        m_Color.clear();
        m_Depth = {};
    }

    void DirectXFramebuffer::Invalidate()
    {
        auto& dx = DirectXContext::Context;

        if (m_Spec.Width == 0 || m_Spec.Height == 0)
        {
            std::cerr << "[DirectXFramebuffer] Invalid size 0x0\n";
            return;
        }

        ReleaseResources();

        const bool msaa = (m_Spec.Samples > 1);

        if (!m_ColorSpecs.empty())
        {
            m_Color.resize(m_ColorSpecs.size());

            for (size_t i = 0; i < m_ColorSpecs.size(); ++i)
            {
                const auto dxFormat = ToDXColorFormat(m_ColorSpecs[i].TextureFormat);
                if (dxFormat == DXGI_FORMAT_UNKNOWN)
                {
                    Q_ERROR("DirectXFramebuffer: Unsupported color format.");
                    continue;
                }

                D3D11_TEXTURE2D_DESC tDesc = {};
                tDesc.Width = m_Spec.Width;
                tDesc.Height = m_Spec.Height;
                tDesc.MipLevels = 1;
                tDesc.ArraySize = 1;
                tDesc.Format = dxFormat;
                tDesc.SampleDesc.Count = msaa ? m_Spec.Samples : 1;
                tDesc.SampleDesc.Quality = 0;
                tDesc.Usage = D3D11_USAGE_DEFAULT;
                tDesc.BindFlags = D3D11_BIND_RENDER_TARGET | (msaa ? 0 : D3D11_BIND_SHADER_RESOURCE);

                HRESULT hr = dx.device->CreateTexture2D(&tDesc, nullptr, m_Color[i].texture.GetAddressOf());
                if (FAILED(hr))
                {
                    Q_ERROR("DirectXFramebuffer: CreateTexture2D(color) failed.");
                    continue;
                }

                D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
                rtvDesc.Format = dxFormat;
                rtvDesc.ViewDimension = msaa ? D3D11_RTV_DIMENSION_TEXTURE2DMS : D3D11_RTV_DIMENSION_TEXTURE2D;
                rtvDesc.Texture2D.MipSlice = 0;

                hr = dx.device->CreateRenderTargetView(m_Color[i].texture.Get(), &rtvDesc, m_Color[i].rtv.GetAddressOf());
                if (FAILED(hr))
                {
                    Q_ERROR("DirectXFramebuffer: CreateRenderTargetView failed.");
                    continue;
                }

                if (!msaa)
                {
                    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
                    srvDesc.Format = dxFormat;
                    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                    srvDesc.Texture2D.MostDetailedMip = 0;
                    srvDesc.Texture2D.MipLevels = 1;

                    hr = dx.device->CreateShaderResourceView(m_Color[i].texture.Get(), &srvDesc, m_Color[i].srv.GetAddressOf());
                    if (FAILED(hr))
                    {
                        Q_ERROR("DirectXFramebuffer: CreateShaderResourceView(color) failed.");
                    }
                }
                else
                {
                    D3D11_TEXTURE2D_DESC rDesc = tDesc;
                    rDesc.SampleDesc.Count = 1;
                    rDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

                    hr = dx.device->CreateTexture2D(&rDesc, nullptr, m_Color[i].resolveTexture.GetAddressOf());
                    if (FAILED(hr))
                    {
                        Q_ERROR("DirectXFramebuffer: CreateTexture2D(resolve) failed.");
                    }
                    else
                    {
                        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
                        srvDesc.Format = dxFormat;
                        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                        srvDesc.Texture2D.MostDetailedMip = 0;
                        srvDesc.Texture2D.MipLevels = 1;
                        hr = dx.device->CreateShaderResourceView(m_Color[i].resolveTexture.Get(), &srvDesc, m_Color[i].resolveSRV.GetAddressOf());
                        if (FAILED(hr))
                        {
                            Q_ERROR("DirectXFramebuffer: CreateShaderResourceView(resolve) failed.");
                        }
                    }
                }
            }
        }

        if (m_DepthSpec.TextureFormat != FramebufferTextureFormat::None)
        {
            D3D11_TEXTURE2D_DESC dDesc = {};
            dDesc.Width = m_Spec.Width;
            dDesc.Height = m_Spec.Height;
            dDesc.MipLevels = 1;
            dDesc.ArraySize = 1;
            dDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
            dDesc.SampleDesc.Count = msaa ? m_Spec.Samples : 1;
            dDesc.SampleDesc.Quality = 0;
            dDesc.Usage = D3D11_USAGE_DEFAULT;
            dDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

            HRESULT hr = dx.device->CreateTexture2D(&dDesc, nullptr, m_Depth.texture.GetAddressOf());
            if (FAILED(hr))
            {
                Q_ERROR("DirectXFramebuffer: CreateTexture2D(depth) failed.");
            }
            else
            {
                D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
                dsvDesc.Format = dDesc.Format;
                dsvDesc.ViewDimension = msaa ? D3D11_DSV_DIMENSION_TEXTURE2DMS : D3D11_DSV_DIMENSION_TEXTURE2D;
                dsvDesc.Texture2D.MipSlice = 0;

                hr = dx.device->CreateDepthStencilView(m_Depth.texture.Get(), &dsvDesc, m_Depth.dsv.GetAddressOf());
                if (FAILED(hr))
                {
                    Q_ERROR("DirectXFramebuffer: CreateDepthStencilView failed.");
                }
            }
        }

        m_Viewport.TopLeftX = 0.0f;
        m_Viewport.TopLeftY = 0.0f;
        m_Viewport.Width = static_cast<float>(m_Spec.Width);
        m_Viewport.Height = static_cast<float>(m_Spec.Height);
        m_Viewport.MinDepth = 0.0f;
        m_Viewport.MaxDepth = 1.0f;
    }

    void DirectXFramebuffer::Resize(uint32_t width, uint32_t height)
    {
        if (width == 0 || height == 0) return;
        if (m_Spec.Width == width && m_Spec.Height == height) return;

        m_Spec.Width = width;
        m_Spec.Height = height;
        Invalidate();
    }

    void DirectXFramebuffer::Resolve()
    {
        if (m_Spec.Samples <= 1) return;

        auto& dx = DirectXContext::Context;
        for (auto& c : m_Color)
        {
            if (c.texture && c.resolveTexture)
            {
                dx.deviceContext->ResolveSubresource(
                    c.resolveTexture.Get(), 0,
                    c.texture.Get(), 0,
                    DXGI_FORMAT_R8G8B8A8_UNORM
                );
            }
        }
    }

    void DirectXFramebuffer::Bind() const
    {
        auto& dx = DirectXContext::Context;

        std::vector<ID3D11RenderTargetView*> rtvs;
        rtvs.reserve(m_Color.size());
        for (auto const& c : m_Color)
            rtvs.push_back(c.rtv.Get());

        dx.deviceContext->OMSetRenderTargets(
            static_cast<UINT>(rtvs.size()),
            rtvs.empty() ? nullptr : rtvs.data(),
            m_Depth.dsv.Get()
        );

        dx.deviceContext->RSSetViewports(1, &m_Viewport);
    }

    void DirectXFramebuffer::Unbind() const
    {
        auto& dx = DirectXContext::Context;
        ID3D11RenderTargetView* nullRTV[8] = { nullptr };
        dx.deviceContext->OMSetRenderTargets(0, nullptr, nullptr);
    }

    void* DirectXFramebuffer::GetColorAttachment(uint32_t index) const
    {
        if (index >= m_Color.size()) return nullptr;

        const bool msaa = (m_Spec.Samples > 1);
        if (msaa)
            return reinterpret_cast<void*>(m_Color[index].resolveSRV.Get());
        else
            return reinterpret_cast<void*>(m_Color[index].srv.Get());
    }

    void* DirectXFramebuffer::GetDepthAttachment() const
    {
        return reinterpret_cast<void*>(m_Depth.dsv.Get());
    }

    int DirectXFramebuffer::ReadPixel(uint32_t attachmentIndex, int x, int y)
    {
        if (attachmentIndex >= m_Color.size()) return -1;

        auto& dx = DirectXContext::Context;
        const bool msaa = (m_Spec.Samples > 1);

        ID3D11Texture2D* srcTex = nullptr;
        if (msaa)
        {
            srcTex = m_Color[attachmentIndex].resolveTexture.Get();
            if (!srcTex) return -1;
        }
        else
        {
            srcTex = m_Color[attachmentIndex].texture.Get();
        }

        D3D11_TEXTURE2D_DESC desc = {};
        srcTex->GetDesc(&desc);
        desc.Usage = D3D11_USAGE_STAGING;
        desc.BindFlags = 0;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        desc.MiscFlags = 0;
        desc.SampleDesc.Count = 1;

        Microsoft::WRL::ComPtr<ID3D11Texture2D> staging;
        HRESULT hr = dx.device->CreateTexture2D(&desc, nullptr, staging.GetAddressOf());
        if (FAILED(hr))
        {
            Q_ERROR("DirectXFramebuffer: ReadPixel CreateTexture2D(staging) failed.");
            return -1;
        }

        D3D11_BOX box;
        box.left = std::clamp(x, 0, (int)m_Spec.Width - 1);
        box.top = std::clamp(y, 0, (int)m_Spec.Height - 1);
        box.right = box.left + 1;
        box.bottom = box.top + 1;
        box.front = 0;
        box.back = 1;

        dx.deviceContext->CopySubresourceRegion(
            staging.Get(), 0, 0, 0, 0,
            srcTex, 0, &box
        );

        D3D11_MAPPED_SUBRESOURCE mapped = {};
        hr = dx.deviceContext->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped);
        if (FAILED(hr))
        {
            Q_ERROR("DirectXFramebuffer: ReadPixel Map failed.");
            return -1;
        }

        const uint8_t* p = reinterpret_cast<const uint8_t*>(mapped.pData);
        int red = p[0];

        dx.deviceContext->Unmap(staging.Get(), 0);
        return red;
    }

    void DirectXFramebuffer::ClearAttachment(uint32_t attachmentIndex, float r, float g, float b, float a)
    {
        if (attachmentIndex >= m_Color.size()) return;

        auto& dx = DirectXContext::Context;
        float color[4] = {
            r, g, b, a
        };
        dx.deviceContext->ClearRenderTargetView(m_Color[attachmentIndex].rtv.Get(), color);
    }

    void DirectXFramebuffer::BindColorAttachment(uint32_t index) const
    {
        if (index >= m_Color.size()) return;
        auto& dx = DirectXContext::Context;

        const bool msaa = (m_Spec.Samples > 1);
        ID3D11ShaderResourceView* srv = msaa ? m_Color[index].resolveSRV.Get()
            : m_Color[index].srv.Get();
        dx.deviceContext->PSSetShaderResources(index, 1, &srv);
    }
}

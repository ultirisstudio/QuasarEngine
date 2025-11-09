#include "qepch.h"
#include "DirectXTexture2D.h"

#include <d3d11.h>
#include <wrl/client.h>
#include "DirectXContext.h"
#include <stb_image.h>
#include <stdexcept>

namespace QuasarEngine
{
    using Microsoft::WRL::ComPtr;

    struct DXTex2DState {
        ComPtr<ID3D11Texture2D> tex;
        ComPtr<ID3D11ShaderResourceView> srv;
        ComPtr<ID3D11SamplerState> sampler;
        int w = 0, h = 0, ch = 0;
        bool loaded = false;
    };
    static std::unordered_map<const DirectXTexture2D*, DXTex2DState> sTex2D;

    DirectXTexture2D::DirectXTexture2D(const TextureSpecification& specification)
        : Texture2D(specification) {
    }

    DirectXTexture2D::~DirectXTexture2D() { sTex2D.erase(this); }

    static void EnsureSampler(DXTex2DState& S) {
        if (S.sampler) return;
        auto& dx = DirectXContext::Context;
        D3D11_SAMPLER_DESC sd{};
        sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        sd.AddressU = sd.AddressV = sd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        sd.MaxLOD = D3D11_FLOAT32_MAX;
        dx.device->CreateSamplerState(&sd, S.sampler.GetAddressOf());
    }

    bool DirectXTexture2D::LoadFromPath(const std::string& path)
    {
        int w, h, c;
        const int desired = 4;
        unsigned char* pixels = stbi_load(path.c_str(), &w, &h, &c, desired);
        if (!pixels) return false;

        bool ok = LoadFromData(ByteView{ pixels, size_t(w * h * desired) });
        stbi_image_free(pixels);
        return ok;
    }

    bool DirectXTexture2D::LoadFromMemory(ByteView bytes)
    {
        int w, h, c;
        const int desired = 4;
        unsigned char* pixels = stbi_load_from_memory(
            (const stbi_uc*)bytes.data, (int)bytes.size, &w, &h, &c, desired);
        if (!pixels) return false;
        bool ok = LoadFromData(ByteView{ pixels, size_t(w * h * desired) });
        stbi_image_free(pixels);
        return ok;
    }

    bool DirectXTexture2D::LoadFromData(ByteView data)
    {
        auto& dx = DirectXContext::Context;
        auto& S = sTex2D[this];

        const int comp = 4;
        const int w = (int)m_Specification.width;
        const int h = (int)m_Specification.height;
        if (w == 0 || h == 0) return false;

        D3D11_TEXTURE2D_DESC td{};
        td.Width = w; td.Height = h;
        td.MipLevels = 1; td.ArraySize = 1;
        td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        td.SampleDesc.Count = 1;
        td.Usage = D3D11_USAGE_DEFAULT;
        td.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA srd{};
        srd.pSysMem = data.data;
        srd.SysMemPitch = w * comp;

        HRESULT hr = dx.device->CreateTexture2D(&td, &srd, S.tex.GetAddressOf());
        if (FAILED(hr)) return false;

        D3D11_SHADER_RESOURCE_VIEW_DESC sv{};
        sv.Format = td.Format;
        sv.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        sv.Texture2D.MipLevels = 1;

        hr = dx.device->CreateShaderResourceView(S.tex.Get(), &sv, S.srv.GetAddressOf());
        if (FAILED(hr)) return false;

        EnsureSampler(S);
        S.w = w; S.h = h; S.ch = comp; S.loaded = true;
        return true;
    }

    void DirectXTexture2D::Bind(int index) const
    {
        auto& dx = DirectXContext::Context;
        auto it = sTex2D.find(this);
        if (it == sTex2D.end()) return;
        ID3D11ShaderResourceView* v = it->second.srv.Get();
        ID3D11SamplerState* s = it->second.sampler.Get();
        dx.deviceContext->PSSetShaderResources(index, 1, &v);
        dx.deviceContext->PSSetSamplers(index, 1, &s);
    }

    void DirectXTexture2D::Unbind() const
    {
        auto& dx = DirectXContext::Context;
        ID3D11ShaderResourceView* nullSRV = nullptr;
        dx.deviceContext->PSSetShaderResources(0, 1, &nullSRV);
    }
}
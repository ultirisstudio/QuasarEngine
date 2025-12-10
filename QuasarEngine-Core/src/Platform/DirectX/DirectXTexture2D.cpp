#include "qepch.h"
#include "DirectXTexture2D.h"

#include <d3d11.h>
#include <wrl/client.h>
#include "DirectXContext.h"
#include <stb_image.h>
#include <stdexcept>
#include <vector>
#include <algorithm>
#include <cstring>
#include <glm/glm.hpp>
#include <glm/common.hpp>

namespace QuasarEngine
{
    using Microsoft::WRL::ComPtr;

    struct DXTex2DState {
        ComPtr<ID3D11Texture2D> tex;
        ComPtr<ID3D11ShaderResourceView> srv;
        ComPtr<ID3D11SamplerState> sampler;
        int  w = 0, h = 0, ch = 0;
        bool loaded = false;

        std::vector<uint8_t> cpuData;
    };
    static std::unordered_map<const DirectXTexture2D*, DXTex2DState> sTex2D;

    DirectXTexture2D::DirectXTexture2D(const TextureSpecification& specification)
        : Texture2D(specification) {
    }

    DirectXTexture2D::~DirectXTexture2D() { sTex2D.erase(this); }

    TextureHandle DirectXTexture2D::GetHandle() const noexcept
    {
        auto it = sTex2D.find(this);
        if (it == sTex2D.end() || !it->second.srv)
            return static_cast<TextureHandle>(0);

        return reinterpret_cast<TextureHandle>(it->second.srv.Get());
    }

    bool DirectXTexture2D::IsLoaded() const noexcept
    {
        auto it = sTex2D.find(this);
        return it != sTex2D.end() && it->second.loaded;
    }

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

        const UINT comp = 4;
        const UINT w = static_cast<UINT>(m_Specification.width);
        const UINT h = static_cast<UINT>(m_Specification.height);

        if (w == 0 || h == 0 || data.data == nullptr)
            return false;

        D3D11_TEXTURE2D_DESC td{};
        td.Width = w;
        td.Height = h;
        td.MipLevels = 1;
        td.ArraySize = 1;
        td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        td.SampleDesc.Count = 1;
        td.SampleDesc.Quality = 0;
        td.Usage = D3D11_USAGE_DEFAULT;
        td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        td.CPUAccessFlags = 0;
        td.MiscFlags = 0;

        D3D11_SUBRESOURCE_DATA srd{};
        srd.pSysMem = data.data;
        srd.SysMemPitch = w * comp;

        HRESULT hr = dx.device->CreateTexture2D(&td, &srd, S.tex.GetAddressOf());
        if (FAILED(hr))
            return false;

        D3D11_SHADER_RESOURCE_VIEW_DESC sv{};
        sv.Format = td.Format;
        sv.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        sv.Texture2D.MostDetailedMip = 0;
        sv.Texture2D.MipLevels = td.MipLevels;

        hr = dx.device->CreateShaderResourceView(S.tex.Get(), &sv, S.srv.GetAddressOf());
        if (FAILED(hr))
            return false;

        EnsureSampler(S);

        S.w = w;
        S.h = h;
        S.ch = comp;
        S.loaded = true;

        const size_t expectedSize = static_cast<size_t>(w) * static_cast<size_t>(h) * static_cast<size_t>(comp);
        const size_t copySize = std::min(expectedSize, static_cast<size_t>(data.size));

        S.cpuData.resize(copySize);
        if (copySize > 0)
            std::memcpy(S.cpuData.data(), data.data, copySize);

        return true;
    }

    glm::vec4 DirectXTexture2D::Sample(const glm::vec2& uv) const
    {
        auto it = sTex2D.find(this);
        if (it == sTex2D.end())
            return glm::vec4(0.0f);

        const DXTex2DState& S = it->second;
        if (!S.loaded || S.w <= 0 || S.h <= 0 || S.ch <= 0 || S.cpuData.empty())
            return glm::vec4(0.0f);

        float u = glm::clamp(uv.x, 0.0f, 1.0f);
        float v = glm::clamp(uv.y, 0.0f, 1.0f);

        int x = static_cast<int>(std::floor(u * float(S.w - 1) + 0.5f));
        int y = static_cast<int>(std::floor(v * float(S.h - 1) + 0.5f));

        x = std::clamp(x, 0, S.w - 1);
        y = std::clamp(y, 0, S.h - 1);

        const size_t idx = (static_cast<size_t>(y) * static_cast<size_t>(S.w) + static_cast<size_t>(x)) * static_cast<size_t>(S.ch);
        if (idx + static_cast<size_t>(S.ch) > S.cpuData.size())
            return glm::vec4(0.0f);

        const uint8_t* p = S.cpuData.data() + idx;
        constexpr float inv255 = 1.0f / 255.0f;

        float r = p[0] * inv255;
        float g = (S.ch > 1 ? p[1] : p[0]) * inv255;
        float b = (S.ch > 2 ? p[2] : p[0]) * inv255;
        float a = (S.ch > 3 ? p[3] : 255u) * inv255;

        return glm::vec4(r, g, b, a);
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

    void DirectXTexture2D::GenerateMips()
    {
        auto& dx = DirectXContext::Context;
        auto it = sTex2D.find(this);
        if (it == sTex2D.end() || !it->second.srv)
            return;

        dx.deviceContext->GenerateMips(it->second.srv.Get());
    }
}
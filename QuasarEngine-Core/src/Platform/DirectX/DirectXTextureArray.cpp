#include "qepch.h"

#include "DirectXTextureArray.h"
#include "DirectXContext.h"

#include <d3d11.h>
#include <wrl/client.h>

#include <stb_image.h>

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace QuasarEngine
{
    using Microsoft::WRL::ComPtr;

    struct DXTexArrayState {
        ComPtr<ID3D11Texture2D> tex;
        ComPtr<ID3D11ShaderResourceView> srv;
        ComPtr<ID3D11SamplerState> sampler;
        bool loaded = false;
    };
    static std::unordered_map<const DirectXTextureArray*, DXTexArrayState> sTexArr;

    DirectXTextureArray::DirectXTextureArray(const TextureSpecification& specification)
        : TextureArray(specification) {
    }

    DirectXTextureArray::~DirectXTextureArray() { sTexArr.erase(this); }

    bool DirectXTextureArray::LoadFromPath(const std::string& path)
    {
        std::vector<std::string> paths;
        paths.emplace_back(path);
        return LoadFromFiles(paths);
    }

    bool DirectXTextureArray::LoadFromMemory(ByteView bytes)
    {
        int w, h, c;
        const int desired = 4;

        stbi_uc* pixels = stbi_load_from_memory(
            reinterpret_cast<const stbi_uc*>(bytes.data),
            static_cast<int>(bytes.size),
            &w, &h, &c, desired);

        if (!pixels)
            return false;

        m_Specification.width = static_cast<uint32_t>(w);
        m_Specification.height = static_cast<uint32_t>(h);

        const bool ok = LoadFromData(ByteView{ pixels, static_cast<size_t>(w * h * desired) });
        stbi_image_free(pixels);
        return ok;
    }

    bool DirectXTextureArray::LoadFromData(ByteView data)
    {
        auto& dx = DirectXContext::Context;
        auto& S = sTexArr[this];

        const UINT W = m_Specification.width;
        const UINT H = m_Specification.height;
        const UINT comp = 4;

        if (W == 0 || H == 0)
            return false;
        if (data.size < static_cast<size_t>(W * H * comp))
            return false;

        D3D11_TEXTURE2D_DESC td{};
        td.Width = W;
        td.Height = H;
        td.MipLevels = 0;
        td.ArraySize = 1;
        td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        td.SampleDesc.Count = 1;
        td.SampleDesc.Quality = 0;
        td.Usage = D3D11_USAGE_DEFAULT;
        td.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
        td.CPUAccessFlags = 0;
        td.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

        Microsoft::WRL::ComPtr<ID3D11Texture2D> tex;
        HRESULT hr = dx.device->CreateTexture2D(&td, nullptr, tex.GetAddressOf());
        if (FAILED(hr))
            return false;

        dx.deviceContext->UpdateSubresource(
            tex.Get(),
            0,
            nullptr,
            data.data,
            W * comp,
            0);

        D3D11_SHADER_RESOURCE_VIEW_DESC srvd{};
        srvd.Format = td.Format;
        srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
        srvd.Texture2DArray.MostDetailedMip = 0;
        srvd.Texture2DArray.MipLevels = -1;
        srvd.Texture2DArray.FirstArraySlice = 0;
        srvd.Texture2DArray.ArraySize = 1;

        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
        hr = dx.device->CreateShaderResourceView(tex.Get(), &srvd, srv.GetAddressOf());
        if (FAILED(hr))
            return false;

        dx.deviceContext->GenerateMips(srv.Get());

        if (!S.sampler) {
            D3D11_SAMPLER_DESC sd{};
            sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
            sd.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
            sd.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
            sd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
            sd.MaxLOD = D3D11_FLOAT32_MAX;
            dx.device->CreateSamplerState(&sd, S.sampler.GetAddressOf());
        }

        S.tex = tex;
        S.srv = srv;
        S.loaded = true;

        m_Specification.width = W;
        m_Specification.height = H;

        return true;
    }

    bool DirectXTextureArray::LoadFromFiles(const std::vector<std::string>& paths)
    {
        if (paths.empty()) return false;

        auto& dx = DirectXContext::Context;
        auto& S = sTexArr[this];

        struct Img { int w = 0, h = 0, c = 0; stbi_uc* pixels = nullptr; };
        std::vector<Img> imgs; imgs.reserve(paths.size());

        int W = 0, H = 0;
        for (const auto& p : paths) {
            Img im;
            im.pixels = stbi_load(p.c_str(), &im.w, &im.h, &im.c, 4);
            if (!im.pixels) {
                for (auto& x : imgs) if (x.pixels) stbi_image_free(x.pixels);
                return false;
            }
            if (imgs.empty()) { W = im.w; H = im.h; }
            if (im.w != W || im.h != H) {
                for (auto& x : imgs) if (x.pixels) stbi_image_free(x.pixels);
                stbi_image_free(im.pixels);
                return false;
            }
            im.c = 4;
            imgs.push_back(im);
        }

        const UINT layers = (UINT)imgs.size();
        const UINT mips = 1u + (UINT)std::floor(std::log2((double)std::max(W, H)));

        D3D11_TEXTURE2D_DESC td{};
        td.Width = (UINT)W;
        td.Height = (UINT)H;
        td.MipLevels = 0;
        td.ArraySize = layers;
        td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        td.SampleDesc.Count = 1;
        td.Usage = D3D11_USAGE_DEFAULT;
        td.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
        td.CPUAccessFlags = 0;
        td.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

        Microsoft::WRL::ComPtr<ID3D11Texture2D> tex;
        HRESULT hr = dx.device->CreateTexture2D(&td, nullptr, tex.GetAddressOf());
        if (FAILED(hr)) {
            for (auto& x : imgs) if (x.pixels) stbi_image_free(x.pixels);
            return false;
        }

        const UINT rowPitch = (UINT)W * 4;
        for (UINT i = 0; i < layers; ++i) {
            const D3D11_BOX box{ 0u, 0u, 0u, (UINT)W, (UINT)H, 1u };
            dx.deviceContext->UpdateSubresource(
                tex.Get(),
                D3D11CalcSubresource(0, i, mips),
                &box,
                imgs[i].pixels,
                rowPitch,
                0u
            );
        }

        D3D11_SHADER_RESOURCE_VIEW_DESC srvd{};
        srvd.Format = td.Format;
        srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
        srvd.Texture2DArray.MostDetailedMip = 0;
        srvd.Texture2DArray.MipLevels = (UINT)-1;
        srvd.Texture2DArray.FirstArraySlice = 0;
        srvd.Texture2DArray.ArraySize = layers;

        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
        hr = dx.device->CreateShaderResourceView(tex.Get(), &srvd, srv.GetAddressOf());
        if (FAILED(hr)) {
            for (auto& x : imgs) if (x.pixels) stbi_image_free(x.pixels);
            return false;
        }

        if (!S.sampler) {
            D3D11_SAMPLER_DESC sd{};
            sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
            sd.AddressU = sd.AddressV = sd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
            sd.MaxLOD = D3D11_FLOAT32_MAX;
            dx.device->CreateSamplerState(&sd, S.sampler.GetAddressOf());
        }

        dx.deviceContext->GenerateMips(srv.Get());

        S.tex = tex;
        S.srv = srv;
        S.loaded = true;

        m_Specification.width = (uint32_t)W;
        m_Specification.height = (uint32_t)H;

        for (auto& x : imgs) if (x.pixels) stbi_image_free(x.pixels);
        return true;
    }

    void DirectXTextureArray::Bind(int index) const
    {
        auto& dx = DirectXContext::Context;
        auto it = sTexArr.find(this);
        if (it == sTexArr.end()) return;
        ID3D11ShaderResourceView* v = it->second.srv.Get();
        ID3D11SamplerState* s = it->second.sampler.Get();
        dx.deviceContext->PSSetShaderResources(index, 1, &v);
        dx.deviceContext->PSSetSamplers(index, 1, &s);
    }
    
    TextureHandle DirectXTextureArray::GetHandle() const noexcept
    {
        auto it = sTexArr.find(this);
        if (it == sTexArr.end() || !it->second.srv)
            return static_cast<TextureHandle>(0);

        return reinterpret_cast<TextureHandle>(it->second.srv.Get());
    }

    bool DirectXTextureArray::IsLoaded() const noexcept
    {
        auto it = sTexArr.find(this);
        return it != sTexArr.end() && it->second.loaded;
    }

    void DirectXTextureArray::GenerateMips()
    {
        auto& dx = DirectXContext::Context;
        auto it = sTexArr.find(this);
        if (it == sTexArr.end() || !it->second.srv)
            return;

        dx.deviceContext->GenerateMips(it->second.srv.Get());
    }

    void DirectXTextureArray::Unbind() const
    {
        auto& dx = DirectXContext::Context;
        ID3D11ShaderResourceView* nullSRV = nullptr;
        dx.deviceContext->PSSetShaderResources(0, 1, &nullSRV);
    }
}
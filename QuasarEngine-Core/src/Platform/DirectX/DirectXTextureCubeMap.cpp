#include "qepch.h"
#include "DirectXTextureCubeMap.h"
#include "DirectXContext.h"
#include <d3d11.h>
#include <wrl/client.h>
#include <stb_image.h>

namespace QuasarEngine
{
    namespace {
        using Microsoft::WRL::ComPtr;

        struct DXTexCubeState {
            ComPtr<ID3D11Texture2D> tex;
            ComPtr<ID3D11ShaderResourceView> srv;
            ComPtr<ID3D11SamplerState> sampler;
            bool loaded = false;
        };

        inline int FaceToIndex(DirectXTextureCubeMap::Face f) {
            return static_cast<int>(f);
        }

        inline UINT CountMips(UINT w, UINT h) {
            UINT m = 1;
            UINT mx = (w > h ? w : h);
            while (mx > 1) { mx >>= 1; ++m; }
            return m;
        }

        struct PendingCube {
            std::array<std::vector<uint8_t>, 6> data{};
            std::array<bool, 6> has{};
            int w = 0, h = 0, ch = 4;
        };

        static std::unordered_map<const QuasarEngine::DirectXTextureCubeMap*, PendingCube> g_Pending;
        static std::unordered_map<const DirectXTextureCubeMap*, DXTexCubeState> sCube;

        inline bool BuildCubeIfReady(const QuasarEngine::DirectXTextureCubeMap* self,
            Microsoft::WRL::ComPtr<ID3D11Texture2D>& outTex,
            Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& outSRV,
            Microsoft::WRL::ComPtr<ID3D11SamplerState>& outSampler)
        {
            auto it = g_Pending.find(self);
            if (it == g_Pending.end()) return false;
            auto& P = it->second;
            for (bool b : P.has) if (!b) return false;

            auto& dx = QuasarEngine::DirectXContext::Context;
            const UINT W = (UINT)P.w, H = (UINT)P.h;
            const UINT mips = CountMips(W, H);

            D3D11_TEXTURE2D_DESC td{};
            td.Width = W;
            td.Height = H;
            td.MipLevels = 0;
            td.ArraySize = 6;
            td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            td.SampleDesc.Count = 1;
            td.Usage = D3D11_USAGE_DEFAULT;
            td.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
            td.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE | D3D11_RESOURCE_MISC_GENERATE_MIPS;

            HRESULT hr = dx.device->CreateTexture2D(&td, nullptr, outTex.GetAddressOf());
            if (FAILED(hr)) return false;

            const UINT rowPitch = W * 4;
            for (UINT face = 0; face < 6; ++face) {
                const D3D11_BOX box{ 0u, 0u, 0u, W, H, 1u };
                dx.deviceContext->UpdateSubresource(
                    outTex.Get(),
                    D3D11CalcSubresource(0, face, mips),
                    &box,
                    P.data[face].data(),
                    rowPitch,
                    0u
                );
            }

            D3D11_SHADER_RESOURCE_VIEW_DESC sv{};
            sv.Format = td.Format;
            sv.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
            sv.TextureCube.MostDetailedMip = 0;
            sv.TextureCube.MipLevels = (UINT)-1;

            hr = dx.device->CreateShaderResourceView(outTex.Get(), &sv, outSRV.GetAddressOf());
            if (FAILED(hr)) return false;

            if (!outSampler) {
                D3D11_SAMPLER_DESC sd{};
                sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
                sd.AddressU = sd.AddressV = sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
                sd.MaxLOD = D3D11_FLOAT32_MAX;
                dx.device->CreateSamplerState(&sd, outSampler.GetAddressOf());
            }

            dx.deviceContext->GenerateMips(outSRV.Get());

            g_Pending.erase(it);
            return true;
        }
    }

    DirectXTextureCubeMap::DirectXTextureCubeMap(const TextureSpecification& specification)
        : TextureCubeMap(specification) {
    }

    DirectXTextureCubeMap::~DirectXTextureCubeMap() { sCube.erase(this); }

    bool DirectXTextureCubeMap::LoadFromPath(const std::string& path)
    {
        bool ok = true;
        ok = ok && LoadFaceFromPath(Face::PositiveX, path);
        ok = ok && LoadFaceFromPath(Face::NegativeX, path);
        ok = ok && LoadFaceFromPath(Face::PositiveY, path);
        ok = ok && LoadFaceFromPath(Face::NegativeY, path);
        ok = ok && LoadFaceFromPath(Face::PositiveZ, path);
        ok = ok && LoadFaceFromPath(Face::NegativeZ, path);
        return ok;
    }

    bool DirectXTextureCubeMap::LoadFromMemory(ByteView bytes)
    {
        int w, h, c;
        const int desired = 4;

        stbi_uc* pixels = stbi_load_from_memory(
            reinterpret_cast<const stbi_uc*>(bytes.data),
            static_cast<int>(bytes.size),
            &w, &h, &c,
            desired);

        if (!pixels)
            return false;

        ByteView faceData{ pixels, static_cast<size_t>(w * h * desired) };

        bool ok = true;
        ok = ok && LoadFaceFromData(Face::PositiveX, faceData, w, h, desired);
        ok = ok && LoadFaceFromData(Face::NegativeX, faceData, w, h, desired);
        ok = ok && LoadFaceFromData(Face::PositiveY, faceData, w, h, desired);
        ok = ok && LoadFaceFromData(Face::NegativeY, faceData, w, h, desired);
        ok = ok && LoadFaceFromData(Face::PositiveZ, faceData, w, h, desired);
        ok = ok && LoadFaceFromData(Face::NegativeZ, faceData, w, h, desired);

        stbi_image_free(pixels);
        return ok;
    }

    bool DirectXTextureCubeMap::LoadFromData(ByteView data)
    {
        const uint32_t w = m_Specification.width;
        const uint32_t h = m_Specification.height;
        const uint32_t channels = 4;

        if (w == 0 || h == 0)
            return false;
        if (data.size < static_cast<size_t>(w * h * channels))
            return false;

        bool ok = true;
        ok = ok && LoadFaceFromData(Face::PositiveX, data, w, h, channels);
        ok = ok && LoadFaceFromData(Face::NegativeX, data, w, h, channels);
        ok = ok && LoadFaceFromData(Face::PositiveY, data, w, h, channels);
        ok = ok && LoadFaceFromData(Face::NegativeY, data, w, h, channels);
        ok = ok && LoadFaceFromData(Face::PositiveZ, data, w, h, channels);
        ok = ok && LoadFaceFromData(Face::NegativeZ, data, w, h, channels);
        return ok;
    }

    bool DirectXTextureCubeMap::LoadFaceFromPath(Face face, const std::string& path)
    {
        int w, h, c;
        stbi_uc* pixels = stbi_load(path.c_str(), &w, &h, &c, 4);
        if (!pixels) return false;

        const bool ok = LoadFaceFromData(face,
            ByteView{ pixels, size_t(w * h * 4) },
            (uint32_t)w, (uint32_t)h, 4);

        stbi_image_free(pixels);
        return ok;
    }

    bool DirectXTextureCubeMap::LoadFaceFromMemory(Face face, ByteView bytes)
    {
        int w, h, c;
        stbi_uc* pixels = stbi_load_from_memory(
            (const stbi_uc*)bytes.data, (int)bytes.size, &w, &h, &c, 4);
        if (!pixels) return false;

        const bool ok = LoadFaceFromData(face,
            ByteView{ pixels, size_t(w * h * 4) },
            (uint32_t)w, (uint32_t)h, 4);

        stbi_image_free(pixels);
        return ok;
    }

    bool DirectXTextureCubeMap::LoadFaceFromData(Face face, ByteView data,
        uint32_t width, uint32_t height, uint32_t channels)
    {
        if (!data.data || data.size == 0) return false;
        if (channels != 4) return false;
        if (width == 0 || height == 0) return false;

        auto& pend = g_Pending[this];
        const int idx = FaceToIndex(face);
        if (idx < 0 || idx >= 6) return false;

        if (!pend.has[0] && !pend.has[1] && !pend.has[2] && !pend.has[3] && !pend.has[4] && !pend.has[5]) {
            pend.w = (int)width; pend.h = (int)height; pend.ch = 4;
        }
        else {
            if (pend.w != (int)width || pend.h != (int)height) return false;
        }

        pend.data[idx].assign((const uint8_t*)data.data, (const uint8_t*)data.data + data.size);
        pend.has[idx] = true;

        auto& dx = DirectXContext::Context;
        auto& S = sCube[this];

        Microsoft::WRL::ComPtr<ID3D11Texture2D> tex;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
        Microsoft::WRL::ComPtr<ID3D11SamplerState> samp = S.sampler;

        const bool built = BuildCubeIfReady(this, tex, srv, samp);
        if (built) {
            S.tex = tex;
            S.srv = srv;
            S.sampler = samp;
            S.loaded = true;

            m_Specification.width = (uint32_t)pend.w;
            m_Specification.height = (uint32_t)pend.h;
        }
        return true;
    }

    void DirectXTextureCubeMap::Bind(int index) const
    {
        auto& dx = DirectXContext::Context;
        auto it = sCube.find(this);
        if (it == sCube.end()) return;
        ID3D11ShaderResourceView* v = it->second.srv.Get();
        ID3D11SamplerState* s = it->second.sampler.Get();
        dx.deviceContext->PSSetShaderResources(index, 1, &v);
        dx.deviceContext->PSSetSamplers(index, 1, &s);
    }

    TextureHandle DirectXTextureCubeMap::GetHandle() const noexcept
    {
        auto it = sCube.find(this);
        if (it == sCube.end() || !it->second.srv)
            return static_cast<TextureHandle>(0);

        return reinterpret_cast<TextureHandle>(it->second.srv.Get());
    }

    bool DirectXTextureCubeMap::IsLoaded() const noexcept
    {
        auto it = sCube.find(this);
        return it != sCube.end() && it->second.loaded;
    }

    void DirectXTextureCubeMap::GenerateMips()
    {
        auto& dx = DirectXContext::Context;
        auto it = sCube.find(this);
        if (it == sCube.end() || !it->second.srv)
            return;

        dx.deviceContext->GenerateMips(it->second.srv.Get());
    }

    void DirectXTextureCubeMap::Unbind() const
    {
        auto& dx = DirectXContext::Context;
        ID3D11ShaderResourceView* nullSRV = nullptr;
        dx.deviceContext->PSSetShaderResources(0, 1, &nullSRV);
    }
}

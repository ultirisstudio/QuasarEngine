#include "qepch.h"
#include "DirectXShader.h"

#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <fstream>
#include <sstream>
#include <glm/gtc/type_ptr.hpp>
#include <QuasarEngine/Core/Logger.h>
#include "DirectXContext.h"

#pragma comment(lib, "d3dcompiler.lib")

namespace QuasarEngine
{
    using Microsoft::WRL::ComPtr;

    namespace {

        DXGI_FORMAT IOTypeToDX(Shader::ShaderIOType t) {
            switch (t) {
            case Shader::ShaderIOType::Float: return DXGI_FORMAT_R32_FLOAT;
            case Shader::ShaderIOType::Vec2:  return DXGI_FORMAT_R32G32_FLOAT;
            case Shader::ShaderIOType::Vec3:  return DXGI_FORMAT_R32G32B32_FLOAT;
            case Shader::ShaderIOType::Vec4:  return DXGI_FORMAT_R32G32B32A32_FLOAT;
            default: return DXGI_FORMAT_UNKNOWN;
            }
        }

        std::string GuessSemantic(std::string name) {
            std::string n = name;
            for (auto& c : n) c = (char)std::tolower(c);
            if (n.find("pos") != std::string::npos)   return "POSITION";
            if (n.find("normal") != std::string::npos)return "NORMAL";
            if (n.find("tex") != std::string::npos ||
                n.find("uv") != std::string::npos)   return "TEXCOORD";
            if (n.find("color") != std::string::npos)   return "COLOR";
            return "TEXCOORD";
        }

        D3D11_FILL_MODE ToDXFill(Shader::FillMode m) {
            switch (m) {
            case Shader::FillMode::Wireframe: return D3D11_FILL_WIREFRAME;
            default:                          return D3D11_FILL_SOLID;
            }
        }

        D3D11_CULL_MODE ToDXCull(Shader::CullMode c) {
            switch (c) {
            case Shader::CullMode::None:  return D3D11_CULL_NONE;
            case Shader::CullMode::Front: return D3D11_CULL_FRONT;
            default:                      return D3D11_CULL_BACK;
            }
        }

        D3D11_COMPARISON_FUNC ToDXDepthFunc(Shader::DepthFunc f) {
            switch (f) {
            case Shader::DepthFunc::Less:         return D3D11_COMPARISON_LESS;
            case Shader::DepthFunc::LessOrEqual:  return D3D11_COMPARISON_LESS_EQUAL;
            case Shader::DepthFunc::Greater:      return D3D11_COMPARISON_GREATER;
            case Shader::DepthFunc::GreaterOrEqual:return D3D11_COMPARISON_GREATER_EQUAL;
            case Shader::DepthFunc::Equal:        return D3D11_COMPARISON_EQUAL;
            case Shader::DepthFunc::NotEqual:     return D3D11_COMPARISON_NOT_EQUAL;
            case Shader::DepthFunc::Always:       return D3D11_COMPARISON_ALWAYS;
            default:                               return D3D11_COMPARISON_LESS;
            }
        }

        struct DXShaderState {
            ComPtr<ID3D11VertexShader> vs;
            ComPtr<ID3D11PixelShader>  ps;
            ComPtr<ID3DBlob>           vsBlob;
            ComPtr<ID3DBlob>           psBlob;
            ComPtr<ID3D11InputLayout>  inputLayout;
            ComPtr<ID3D11RasterizerState> rs;
            ComPtr<ID3D11DepthStencilState> dss;
            ComPtr<ID3D11BlendState>   bs;

            std::unordered_map<std::string, uint32_t> samplerSlots;
        };

        static std::unordered_map<const DirectXShader*, DXShaderState> sStates;

        void CreatePipelineStates(const Shader::ShaderDescription& d, DXShaderState& S) {
            auto& dx = DirectXContext::Context;

            D3D11_RASTERIZER_DESC rd{};
            rd.FillMode = ToDXFill(d.fillMode);
            rd.CullMode = ToDXCull(d.cullMode);
            rd.DepthClipEnable = TRUE;
            dx.device->CreateRasterizerState(&rd, S.rs.GetAddressOf());

            D3D11_DEPTH_STENCIL_DESC dd{};
            dd.DepthEnable = d.depthTestEnable ? TRUE : FALSE;
            dd.DepthWriteMask = d.depthWriteEnable ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
            dd.DepthFunc = ToDXDepthFunc(d.depthFunc);
            dx.device->CreateDepthStencilState(&dd, S.dss.GetAddressOf());

            D3D11_BLEND_DESC bd{};
            bd.AlphaToCoverageEnable = FALSE;
            bd.IndependentBlendEnable = FALSE;
            auto& rt = bd.RenderTarget[0];
            if (d.blendMode == Shader::BlendMode::None) {
                rt.BlendEnable = FALSE;
            }
            else {
                rt.BlendEnable = TRUE;
                rt.SrcBlend = D3D11_BLEND_SRC_ALPHA;
                rt.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
                rt.BlendOp = D3D11_BLEND_OP_ADD;
                rt.SrcBlendAlpha = D3D11_BLEND_ONE;
                rt.DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
                rt.BlendOpAlpha = D3D11_BLEND_OP_ADD;
            }
            rt.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
            dx.device->CreateBlendState(&bd, S.bs.GetAddressOf());
        }

        void CreateInputLayout(const Shader::ShaderDescription& d, DXShaderState& S) {
            if (!S.vsBlob) return;
            auto& dx = DirectXContext::Context;

            std::vector<D3D11_INPUT_ELEMENT_DESC> elems;
            UINT texIndex = 0, colIndex = 0;

            for (auto const& mod : d.modules) {
                if (mod.stage != Shader::ShaderStageType::Vertex) continue;
                elems.reserve(mod.inputs.size());
                for (auto const& in : mod.inputs) {
                    D3D11_INPUT_ELEMENT_DESC e{};
                    const std::string semantic = GuessSemantic(in.name);
                    e.SemanticName = semantic.c_str();
                    if (semantic == "TEXCOORD") e.SemanticIndex = texIndex++;
                    else if (semantic == "COLOR") e.SemanticIndex = colIndex++;
                    else e.SemanticIndex = 0;
                    e.Format = IOTypeToDX(in.type);
                    e.InputSlot = 0;
                    e.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
                    e.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
                    e.InstanceDataStepRate = 0;
                    elems.push_back(e);
                }
                break;
            }
            dx.device->CreateInputLayout(
                elems.data(), (UINT)elems.size(),
                S.vsBlob->GetBufferPointer(), S.vsBlob->GetBufferSize(),
                S.inputLayout.GetAddressOf());
        }

        ComPtr<ID3DBlob> Compile(const std::string& path, const char* entry, const char* profile)
        {
            UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(_DEBUG)
            flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
            flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif
            ComPtr<ID3DBlob> blob, err;
            HRESULT hr = D3DCompileFromFile(
                std::wstring(path.begin(), path.end()).c_str(),
                nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
                entry, profile, flags, 0, blob.GetAddressOf(), err.GetAddressOf());
            if (FAILED(hr)) {
                if (err) Q_ERROR(std::string("HLSL compile error: ") + (char*)err->GetBufferPointer());
                throw std::runtime_error("Failed to compile HLSL: " + path);
            }
            return blob;
        }
    }

    std::string DirectXShader::ReadFile(const std::string& path)
    {
        std::ifstream file(path);
        if (!file.is_open())
            throw std::runtime_error("Failed to open shader file: " + path);

        std::stringstream ss;
        ss << file.rdbuf();
        return ss.str();
    }

    uint32_t DirectXShader::CompileShader(const std::string&, uint32_t) { return 0; }

    void DirectXShader::LinkProgram(const std::vector<uint32_t>&) {}

    void DirectXShader::ExtractUniformLocations()
    {
        size_t gSize = 0, oSize = 0;
        for (auto const& u : m_Description.globalUniforms)
            gSize = std::max(gSize, size_t(u.offset + u.size));
        for (auto const& u : m_Description.objectUniforms)
            oSize = std::max(oSize, size_t(u.offset + u.size));

        m_GlobalUniformData.resize((gSize + 15) & ~size_t(15));
        m_ObjectUniformData.resize((oSize + 15) & ~size_t(15));

        for (auto const& u : m_Description.globalUniforms)
            m_GlobalUniformMap[u.name] = &u;
        for (auto const& u : m_Description.objectUniforms)
            m_ObjectUniformMap[u.name] = &u;

        m_GlobalUBO = std::make_unique<DirectXUniformBuffer>(m_GlobalUniformData.size(), 0);
        m_ObjectUBO = std::make_unique<DirectXUniformBuffer>(m_ObjectUniformData.size(), 1);
    }

    DirectXShader::DirectXShader(const ShaderDescription& desc)
        : m_Description(desc)
    {
        auto& S = sStates[this];
        auto& dx = DirectXContext::Context;

        for (auto const& mod : m_Description.modules) {
            if (mod.stage == Shader::ShaderStageType::Vertex) {
                auto blob = Compile(mod.path, "main", "vs_5_0");
                dx.device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, S.vs.GetAddressOf());
                S.vsBlob = blob;
            }
            else if (mod.stage == Shader::ShaderStageType::Fragment) {
                auto blob = Compile(mod.path, "main", "ps_5_0");
                dx.device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, S.ps.GetAddressOf());
                S.psBlob = blob;
            }
        }

        CreateInputLayout(m_Description, S);
        CreatePipelineStates(m_Description, S);

        for (auto const& s : m_Description.samplers)
            S.samplerSlots[s.name] = s.binding;

        ExtractUniformLocations();
    }

    DirectXShader::~DirectXShader()
    {
        sStates.erase(this);
    }

    void DirectXShader::Use()
    {
        ApplyPipelineStates();
    }

    void DirectXShader::Unuse()
    {
        
    }

    void DirectXShader::Reset()
    {
        
    }

    bool DirectXShader::UpdateGlobalState()
    {
        if (m_GlobalUBO && !m_GlobalUniformData.empty())
            m_GlobalUBO->SetData(m_GlobalUniformData.data(), m_GlobalUniformData.size());
        return true;
    }

    bool DirectXShader::UpdateObject(Material*)
    {
        if (m_ObjectUBO && !m_ObjectUniformData.empty())
            m_ObjectUBO->SetData(m_ObjectUniformData.data(), m_ObjectUniformData.size());

        auto& dx = DirectXContext::Context;
        auto& S = sStates[this];

        for (auto const& kv : m_ObjectTextures) {
            const std::string& name = kv.first;
            auto* tex = kv.second;
            auto it = S.samplerSlots.find(name);
            const UINT slot = (it != S.samplerSlots.end()) ? it->second : 0;
            tex->Bind((int)slot);
        }
        return true;
    }

    bool DirectXShader::SetUniform(const std::string& name, void* data, size_t size)
    {
        if (!data || size == 0) return false;

        if (auto it = m_GlobalUniformMap.find(name); it != m_GlobalUniformMap.end()) {
            const auto* u = it->second;
            std::memcpy(m_GlobalUniformData.data() + u->offset, data, std::min(size_t(u->size), size));
            return true;
        }
        if (auto it = m_ObjectUniformMap.find(name); it != m_ObjectUniformMap.end()) {
            const auto* u = it->second;
            std::memcpy(m_ObjectUniformData.data() + u->offset, data, std::min(size_t(u->size), size));
            return true;
        }
        return false;
    }

    bool DirectXShader::SetTexture(const std::string& name, Texture* texture, SamplerType)
    {
        m_ObjectTextures[name] = static_cast<DirectXTexture2D*>(texture);
        return true;
    }

    bool DirectXShader::SetStorageBuffer(const std::string& name, const void* data, size_t size)
    {
        if (!data || size == 0)
            return false;

        auto& info = m_StorageBuffers[name];

        if (!info.buffer)
        {
            info.binding = m_NextStorageBinding++;
            info.buffer = std::make_unique<DirectXUniformBuffer>(size, info.binding);
        }
        else if (size > info.buffer->GetSize())
        {
            info.buffer = std::make_unique<DirectXUniformBuffer>(size, info.binding);
        }

        info.buffer->SetData(data, size);
        return true;
    }

    void DirectXShader::ApplyPipelineStates()
    {
        auto& dx = DirectXContext::Context;
        auto& S = sStates[this];

        dx.deviceContext->IASetInputLayout(S.inputLayout.Get());
        dx.deviceContext->VSSetShader(S.vs.Get(), nullptr, 0);
        dx.deviceContext->PSSetShader(S.ps.Get(), nullptr, 0);
        dx.deviceContext->RSSetState(S.rs.Get());
        dx.deviceContext->OMSetDepthStencilState(S.dss.Get(), 0);

        float blendFactor[4] = { 0,0,0,0 };
        dx.deviceContext->OMSetBlendState(S.bs.Get(), blendFactor, 0xffffffff);
    }
}
#include "qepch.h"
#include "DirectXRendererAPI.h"

#include <d3d11.h>
#include <wrl/client.h>
#include "DirectXContext.h"
#include "QuasarEngine/Renderer/DrawMode.h"

namespace QuasarEngine {

    namespace {
        D3D11_PRIMITIVE_TOPOLOGY ToDXTopology(DrawMode m) {
            switch (m) {
            case DrawMode::POINTS:         return D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
            case DrawMode::LINES:          return D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
            case DrawMode::LINE_STRIP:     return D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
            case DrawMode::TRIANGLES:      return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
            case DrawMode::TRIANGLE_STRIP: return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
            default:                       return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
            }
        }

        float g_ClearColor[4] = { 0.1f, 0.1f, 0.3f, 1.0f };
    }

    void DirectXRendererAPI::Initialize()
    {
        auto& dx = DirectXContext::Context;
        D3D11_VIEWPORT vp{};
        vp.TopLeftX = 0.0f; vp.TopLeftY = 0.0f;
        vp.Width = static_cast<float>(dx.width);
        vp.Height = static_cast<float>(dx.height);
        vp.MinDepth = 0.0f; vp.MaxDepth = 1.0f;
        dx.deviceContext->RSSetViewports(1, &vp);
    }

    void DirectXRendererAPI::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
    {
        auto& dx = DirectXContext::Context;
        D3D11_VIEWPORT vp{};
        vp.TopLeftX = static_cast<float>(x);
        vp.TopLeftY = static_cast<float>(y);
        vp.Width = static_cast<float>(width);
        vp.Height = static_cast<float>(height);
        vp.MinDepth = 0.0f; vp.MaxDepth = 1.0f;
        dx.deviceContext->RSSetViewports(1, &vp);
    }

    void DirectXRendererAPI::ClearColor(const glm::vec4& color)
    {
        g_ClearColor[0] = color.r;
        g_ClearColor[1] = color.g;
        g_ClearColor[2] = color.b;
        g_ClearColor[3] = color.a;
    }

    void DirectXRendererAPI::Clear()
    {
        auto& dx = DirectXContext::Context;
        if (dx.renderTargetView)
            dx.deviceContext->ClearRenderTargetView(dx.renderTargetView.Get(), g_ClearColor);
    }

void DirectXRendererAPI::DrawArrays(DrawMode drawMode, uint32_t size)
{
auto& dx = DirectXContext::Context;
dx.deviceContext->IASetPrimitiveTopology(ToDXTopology(drawMode));
dx.deviceContext->Draw(size, 0);
}

void DirectXRendererAPI::DrawArraysInstanced(DrawMode drawMode, uint32_t size, uint32_t instanceCount)
{
auto& dx = DirectXContext::Context;
dx.deviceContext->IASetPrimitiveTopology(ToDXTopology(drawMode));
dx.deviceContext->DrawInstanced(size, instanceCount, 0, 0);
}

void DirectXRendererAPI::DrawElements(DrawMode drawMode, uint32_t count, uint32_t firstIndex, int32_t baseVertex)
{
auto& dx = DirectXContext::Context;
dx.deviceContext->IASetPrimitiveTopology(ToDXTopology(drawMode));
dx.deviceContext->DrawIndexed(count, firstIndex, baseVertex);
}

void DirectXRendererAPI::DrawElementsInstanced(DrawMode drawMode, uint32_t count, uint32_t instanceCount, uint32_t firstIndex, int32_t baseVertex)
{
auto& dx = DirectXContext::Context;
dx.deviceContext->IASetPrimitiveTopology(ToDXTopology(drawMode));
dx.deviceContext->DrawIndexedInstanced(count, instanceCount, firstIndex, baseVertex, 0);
}
}

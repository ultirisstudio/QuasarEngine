#include "qepch.h"
#include "DirectXVertexArray.h"
#include "DirectXBuffer.h"
#include "DirectXContext.h"

namespace QuasarEngine
{
    DirectXVertexArray::DirectXVertexArray() = default;
    DirectXVertexArray::~DirectXVertexArray() = default;

    void DirectXVertexArray::Bind() const
    {
        for (auto const& vb : m_VertexBuffers) vb->Bind();
        if (m_IndexBuffer) m_IndexBuffer->Bind();
    }

    void DirectXVertexArray::Unbind() const
    {
        auto& dx = DirectXContext::Context;
        ID3D11Buffer* nullBuf = nullptr; UINT s = 0, o = 0;
        dx.deviceContext->IASetVertexBuffers(0, 1, &nullBuf, &s, &o);
        dx.deviceContext->IASetIndexBuffer(nullptr, DXGI_FORMAT_R32_UINT, 0);
    }

    void DirectXVertexArray::AddVertexBuffer(const std::shared_ptr<VertexBuffer>& vertexBuffer)
    {
        m_VertexBuffers.push_back(vertexBuffer);
    }

    void DirectXVertexArray::SetIndexBuffer(const std::shared_ptr<IndexBuffer>& indexBuffer)
    {
        m_IndexBuffer = indexBuffer;
    }
}
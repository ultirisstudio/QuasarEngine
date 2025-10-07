#include "qepch.h"
#include "ScreenQuad.h"

#include <QuasarEngine/Renderer/RenderCommand.h>
#include <QuasarEngine/Renderer/Buffer.h>

namespace QuasarEngine
{
    ScreenQuad::ScreenQuad()
    {
        std::vector<float> vertices = {
            -1.0f, -1.0f,  0.0f, 0.0f,
             1.0f, -1.0f,  1.0f, 0.0f,
             1.0f,  1.0f,  1.0f, 1.0f,
            -1.0f,  1.0f,  0.0f, 1.0f
        };

        std::vector<unsigned int> indices = {
            0, 1, 2,
            0, 2, 3
        };

        m_vertexArray = VertexArray::Create();

        m_vertexBuffer = VertexBuffer::Create(vertices.data(), vertices.size() * sizeof(float));
        m_vertexBuffer->SetLayout({
            { ShaderDataType::Vec2, "aPos"				},
            { ShaderDataType::Vec2, "aTexCoords"		}
        });
        m_vertexArray->AddVertexBuffer(m_vertexBuffer);

        std::shared_ptr<IndexBuffer> indexBuffer = IndexBuffer::Create(indices.data(), indices.size() * sizeof(unsigned int));
        m_vertexArray->SetIndexBuffer(indexBuffer);
    }

    ScreenQuad::~ScreenQuad()
    {
        
    }

    void ScreenQuad::Draw() const
    {
        RenderCommand::Instance().Clear();
        RenderCommand::Instance().ClearColor({ 1.0f, 1.0f, 1.0f, 1.0f });

        m_vertexArray->Bind();

        uint32_t count = m_vertexArray->GetIndexBuffer()->GetCount();

        RenderCommand::Instance().DrawElements(DrawMode::TRIANGLES, count);
    }
}
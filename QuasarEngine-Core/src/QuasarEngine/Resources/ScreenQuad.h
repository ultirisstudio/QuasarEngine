#pragma once

#include <QuasarEngine/Renderer/VertexArray.h>
#include <QuasarEngine/Renderer/Buffer.h>

namespace QuasarEngine
{
    class ScreenQuad
    {
    private:
        std::shared_ptr<VertexArray> m_vertexArray;
        std::shared_ptr<VertexBuffer> m_vertexBuffer;

    public:
        ScreenQuad();
        ~ScreenQuad();

        void Draw() const;
    };
}
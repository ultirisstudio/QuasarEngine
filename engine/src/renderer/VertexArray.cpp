#include <quasar/renderer/VertexArray.h>

#include <quasar/log/Log.h>

#include <bitset>
#include <cstdint>
#include <utility>

namespace Quasar
{
    void VertexArray::AddVertexBuffer(Scope<VertexBuffer> buffer)
    {
        if (buffer == nullptr)
        {
            QUASAR_ENGINE_ERROR("Cannot add a null vertex buffer to a vertex array");
            return;
        }

        std::bitset<VertexLayout::MaxLocations> locations;
        for (const VertexAttribute& attribute : buffer->GetLayout().GetAttributes())
        {
            for (std::uint32_t column = 0; column < GetVertexAttributeLocationCount(attribute.Type); ++column)
            {
                locations.set(attribute.Location + column);
            }
        }
        if ((locations & m_UsedLocations).any())
        {
            QUASAR_ENGINE_ERROR("Cannot add a vertex buffer to a vertex array: its layout uses locations that another vertex buffer of the array uses");
            return;
        }

        AttachVertexBuffer(*buffer, static_cast<std::uint32_t>(m_VertexBuffers.size()));
        m_UsedLocations |= locations;
        m_VertexBuffers.push_back(std::move(buffer));
    }

    void VertexArray::SetIndexBuffer(Scope<IndexBuffer> buffer)
    {
        if (buffer == nullptr)
        {
            QUASAR_ENGINE_ERROR("Cannot set a null index buffer in a vertex array");
            return;
        }

        AttachIndexBuffer(*buffer);
        m_IndexBuffer = std::move(buffer);
    }
}

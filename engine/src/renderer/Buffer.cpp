#include <quasar/renderer/Buffer.h>

#include <quasar/log/Log.h>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <utility>

namespace Quasar
{
    namespace
    {
        constexpr auto MaxIndexCount = static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max());

        bool Fits(std::size_t count, std::size_t first, std::size_t capacity)
        {
            return count <= capacity && first <= capacity - count;
        }
    }

    VertexBuffer::VertexBuffer(std::size_t size, VertexLayout layout) : m_Size(size), m_Layout(std::move(layout)) {}

    bool VertexBuffer::IsValid(std::size_t size, const VertexLayout& layout)
    {
        if (!layout.IsValid())
        {
            return false;
        }
        if (size == 0)
        {
            QUASAR_ENGINE_ERROR("Cannot create an empty vertex buffer");
            return false;
        }
        if (size % layout.GetStride() != 0)
        {
            QUASAR_ENGINE_ERROR("Cannot create a vertex buffer of {} bytes: it is not a whole number of vertices of {} bytes", size, layout.GetStride());
            return false;
        }
        return true;
    }

    void VertexBuffer::SetData(std::span<const std::byte> data, std::size_t offset)
    {
        if (!Fits(data.size(), offset, m_Size))
        {
            QUASAR_ENGINE_ERROR("Cannot write {} bytes at the offset {} of a vertex buffer of {} bytes", data.size(), offset, m_Size);
            return;
        }
        if (!data.empty())
        {
            Write(data, offset);
        }
    }

    IndexBuffer::IndexBuffer(std::uint32_t count) : m_Count(count) {}

    bool IndexBuffer::IsValid(std::size_t count)
    {
        if (count == 0)
        {
            QUASAR_ENGINE_ERROR("Cannot create an empty index buffer");
            return false;
        }
        if (count > MaxIndexCount)
        {
            QUASAR_ENGINE_ERROR("Cannot create an index buffer of {} indices: the most is {}", count, MaxIndexCount);
            return false;
        }
        return true;
    }

    void IndexBuffer::SetData(std::span<const std::uint32_t> indices, std::size_t first)
    {
        if (!Fits(indices.size(), first, m_Count))
        {
            QUASAR_ENGINE_ERROR("Cannot write {} indices from the index {} of an index buffer of {} indices", indices.size(), first, m_Count);
            return;
        }
        if (!indices.empty())
        {
            Write(indices, first);
        }
    }

    UniformBuffer::UniformBuffer(std::size_t size, std::uint32_t binding) : m_Size(size), m_Binding(binding) {}

    bool UniformBuffer::IsValid(std::size_t size)
    {
        if (size == 0)
        {
            QUASAR_ENGINE_ERROR("Cannot create an empty uniform buffer");
            return false;
        }
        return true;
    }

    void UniformBuffer::SetData(std::span<const std::byte> data, std::size_t offset)
    {
        if (!Fits(data.size(), offset, m_Size))
        {
            QUASAR_ENGINE_ERROR("Cannot write {} bytes at the offset {} of a uniform buffer of {} bytes", data.size(), offset, m_Size);
            return;
        }
        if (!data.empty())
        {
            Write(data, offset);
        }
    }

    FrameUniformBuffer::FrameUniformBuffer(std::size_t partSize, std::uint32_t binding) : m_PartSize(partSize), m_Binding(binding) {}

    bool FrameUniformBuffer::IsValid(std::size_t partSize)
    {
        if (partSize == 0)
        {
            QUASAR_ENGINE_ERROR("Cannot create a frame uniform buffer with empty parts");
            return false;
        }
        return true;
    }

    std::optional<std::uint32_t> FrameUniformBuffer::AddData(std::span<const std::byte> data)
    {
        if (data.size() != m_PartSize)
        {
            QUASAR_ENGINE_ERROR("Cannot add a part of {} bytes to a frame uniform buffer with parts of {} bytes", data.size(), m_PartSize);
            return std::nullopt;
        }
        return Write(data);
    }

    void FrameUniformBuffer::Bind(std::uint32_t part)
    {
        if (part >= GetPartCount())
        {
            QUASAR_ENGINE_ERROR("Cannot bind the part {} of a frame uniform buffer: the current frame added {} parts", part, GetPartCount());
            return;
        }
        BindPart(part);
    }
}

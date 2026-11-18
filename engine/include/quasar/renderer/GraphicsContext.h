#pragma once

/// \file
/// \brief The connection between a window and the graphics API that draws in it.
/// \author Kurogenshi
/// \date 2026-11-10

#include <quasar/core/Base.h>
#include <quasar/renderer/Buffer.h>
#include <quasar/renderer/Framebuffer.h>
#include <quasar/renderer/GraphicsAPI.h>
#include <quasar/renderer/Shader.h>
#include <quasar/renderer/VertexArray.h>
#include <quasar/renderer/VertexLayout.h>

#include <cstddef>
#include <cstdint>
#include <span>

namespace Quasar
{
    /// @brief Connection between a window and its graphics API, independent from the API.
    ///
    /// The window creates its context from WindowSpecification::Graphics and owns it. Each graphics API has its own
    /// class in `platform/<api>`, for example OpenGLContext. Code that needs the API itself converts the context to that
    /// class after checking GetAPI().
    class GraphicsContext
    {
    public:
        GraphicsContext() = default;
        virtual ~GraphicsContext() = default;

        GraphicsContext(const GraphicsContext&) = delete;
        GraphicsContext& operator=(const GraphicsContext&) = delete;
        GraphicsContext(GraphicsContext&&) = delete;
        GraphicsContext& operator=(GraphicsContext&&) = delete;

        /// @brief Returns the graphics API of the context.
        virtual GraphicsAPI GetAPI() const = 0;

        /// @brief Shows the image drawn since the last call in the window.
        virtual void SwapBuffers() = 0;

        /// @brief Enables or disables vertical synchronization, which makes SwapBuffers() wait for the monitor.
        virtual void SetVSync(bool enabled) = 0;

        /// @brief Creates a framebuffer that draws with the graphics API of the context.
        /// @return The framebuffer, or nullptr if the specification is not valid or if the graphics API cannot create it.
        /// The reason is logged as an engine error.
        [[nodiscard]] virtual Scope<Framebuffer> CreateFramebuffer(const FramebufferSpecification& specification) = 0;

        /// @brief Compiles a shader for the graphics API of the context.
        /// @return The shader, or nullptr if the specification is not valid or if a stage cannot be compiled or linked.
        /// The reason, with the messages of the compiler, is logged as an engine error.
        [[nodiscard]] virtual Scope<Shader> CreateShader(const ShaderSpecification& specification) = 0;

        /// @brief Creates a vertex buffer from its vertices, which also give its size.
        /// @param vertices The bytes of the vertices, for example `std::as_bytes(std::span(vertices))`.
        /// @param layout How the attributes of a vertex are stored.
        /// @return The buffer, or nullptr if the size or the layout is not valid, see VertexBuffer::IsValid(). The reason
        /// is logged as an engine error.
        [[nodiscard]] virtual Scope<VertexBuffer> CreateVertexBuffer(std::span<const std::byte> vertices, const VertexLayout& layout) = 0;

        /// @brief Creates an index buffer from its indices, which also give its size.
        /// @return The buffer, or nullptr if there are no indices. The reason is logged as an engine error.
        [[nodiscard]] virtual Scope<IndexBuffer> CreateIndexBuffer(std::span<const std::uint32_t> indices) = 0;

        /// @brief Creates a uniform buffer filled with zeros.
        /// @param size The size of the buffer, in bytes, usually the size of the struct that fills it.
        /// @param binding The N of `layout(std140, binding = N)` of the uniform block that the buffer fills.
        /// @return The buffer, or nullptr if the size is 0 or if the graphics API does not allow the size or the
        /// binding. The reason is logged as an engine error.
        [[nodiscard]] virtual Scope<UniformBuffer> CreateUniformBuffer(std::size_t size, std::uint32_t binding) = 0;

        /// @brief Creates a vertex array without buffers.
        [[nodiscard]] virtual Scope<VertexArray> CreateVertexArray() = 0;

        /// @brief Draws triangles from the first vertices of a vertex array, with the bound shader, into the bound
        /// framebuffer or the window.
        /// @param vertexArray The vertex array, created by this context.
        /// @param vertexCount The number of vertices, 3 per triangle. More vertices than a vertex buffer of the array
        /// holds are refused with an engine error.
        virtual void Draw(const VertexArray& vertexArray, std::uint32_t vertexCount) = 0;

        /// @brief Draws the triangles of the index buffer of a vertex array, with the bound shader, into the bound
        /// framebuffer or the window. A vertex array without an index buffer is refused with an engine error.
        /// @param vertexArray The vertex array, created by this context. Its indices must be below the number of
        /// vertices of its vertex buffers.
        virtual void DrawIndexed(const VertexArray& vertexArray) = 0;
    };
}

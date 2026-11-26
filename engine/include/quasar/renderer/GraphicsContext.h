#pragma once

/// \file
/// \brief The connection between a window and the graphics API that draws in it.
/// \author Kurogenshi
/// \date 2026-11-10

#include <quasar/core/Base.h>
#include <quasar/renderer/Buffer.h>
#include <quasar/renderer/Framebuffer.h>
#include <quasar/renderer/GpuProfiler.h>
#include <quasar/renderer/GraphicsAPI.h>
#include <quasar/renderer/Pipeline.h>
#include <quasar/renderer/RenderStatistics.h>
#include <quasar/renderer/Shader.h>
#include <quasar/renderer/Texture.h>
#include <quasar/renderer/TimestampQuery.h>
#include <quasar/renderer/VertexArray.h>
#include <quasar/renderer/VertexLayout.h>

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
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

        /// @brief Creates a pipeline: a shader and the state of its draws.
        /// @return The pipeline, or nullptr if the specification has no shader. The reason is logged as an engine error.
        [[nodiscard]] virtual Scope<Pipeline> CreatePipeline(const PipelineSpecification& specification) = 0;

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

        /// @brief Creates a uniform buffer written again at each frame, in parts of one size. See FrameUniformBuffer.
        /// @param partSize The size of a part, in bytes, usually the size of the struct that fills it.
        /// @param binding The N of `layout(std140, binding = N)` of the uniform block that the buffer fills.
        /// @return The buffer, or nullptr if the size is 0 or if the graphics API does not allow the size or the
        /// binding. The reason is logged as an engine error.
        [[nodiscard]] virtual Scope<FrameUniformBuffer> CreateFrameUniformBuffer(std::size_t partSize, std::uint32_t binding) = 0;

        /// @brief Creates a 2D texture.
        /// @param specification The size and the settings of the texture.
        /// @param pixels Width * Height pixels of GetTexturePixelSize() bytes, from the top row, for example the
        /// Image::Pixels of an image of the same size. Without pixels, the texture is filled with zeros.
        /// @return The texture, or nullptr if the specification is not valid, if the pixels do not have the size of the
        /// texture, or if the graphics API does not allow its size. The reason is logged as an engine error.
        [[nodiscard]] virtual Scope<Texture2D> CreateTexture2D(const TextureSpecification& specification, std::span<const std::byte> pixels) = 0;

        /// @brief Creates a 2D texture from the image of a PNG, JPEG, BMP or TGA file. See Image::FromFile().
        /// @param file The file.
        /// @param settings The settings of the texture, whose size is replaced by the size of the image. Its format must
        /// be TextureFormat::Srgba8, the default, for an image of colors, or TextureFormat::Rgba8 for an image of
        /// data. See TextureSpecification::Format.
        /// @return The texture, or nullptr if the file cannot be decoded or if the texture cannot be created. The reason
        /// is logged as an engine error.
        [[nodiscard]] Scope<Texture2D> LoadTexture2D(const std::filesystem::path& file, TextureSpecification settings = {});

        /// @brief Returns a texture of 1 x 1 white pixel, made at the first call and owned by the context, for example to
        /// read when a texture is not set. It must not be changed.
        /// @return The texture, or nullptr if the graphics API cannot create it. The reason is logged as an engine
        /// error, and the next call tries again.
        Texture2D* GetWhiteTexture();

        /// @brief Creates a vertex array without buffers.
        [[nodiscard]] virtual Scope<VertexArray> CreateVertexArray() = 0;

        /// @brief Creates a query that reads the clock of the graphics card, to measure the time of its commands.
        /// @return The query, or nullptr if the graphics API cannot create it. The reason is logged as an engine error.
        [[nodiscard]] virtual Scope<TimestampQuery> CreateTimestampQuery() = 0;

        /// @brief Returns the profiler of the commands of this context, made at the first call and owned by the
        /// context. It is off until GpuProfiler::SetEnabled() turns it on.
        GpuProfiler& GetProfiler();

        /// @brief Returns what the engine sent to the graphics card during the last frame that EndFrame() ended.
        const RenderStatistics& GetFrameStatistics() const { return m_FrameStatistics; }

        /// @brief Ends the frame: GetFrameStatistics() gives its counts, the next frame is counted from zero, and the
        /// FrameUniformBuffer objects start again from their part 0. Application calls it after each rendered frame.
        void EndFrame();

        /// @brief Returns the number of frames that EndFrame() ended, which identifies the current frame.
        std::uint64_t GetFrameNumber() const { return m_FrameNumber; }

        /// @brief Tells the context that the state of the graphics API was changed outside of the engine, for example by
        /// direct OpenGL calls or by the ImGui backend. The context remembers what is bound and set to skip what is
        /// already in place: after this call, it sends again every bind and every change.
        virtual void InvalidateState() = 0;

        /// @brief Makes the next draws go into the window, on its whole area.
        virtual void BindWindow() = 0;

        /// @brief Returns the size of the drawing area of the window, in pixels, which is the size of the images drawn
        /// into the window. See Window::GetWidth().
        virtual glm::uvec2 GetWindowSize() const = 0;

        /// @brief Returns the most samples per pixel of a framebuffer with color and depth attachments, for antialiasing.
        /// See FramebufferSpecification::Samples.
        virtual std::uint32_t GetMaxSamples() const = 0;

        /// @brief Returns the most anisotropy of a texture, or 1 if the graphics driver does not support anisotropic
        /// filtering. See TextureSpecification::Anisotropy.
        virtual float GetMaxAnisotropy() const = 0;

        /// @brief Fills the window with a color, and its depth and stencil with a value, whatever the pipeline bound.
        virtual void ClearWindow(glm::vec4 color, float depth = 1.0f, std::uint8_t stencil = 0) = 0;

        /// @brief Draws triangles from the first vertices of a vertex array, with the bound pipeline or shader, into the
        /// bound framebuffer or the window.
        /// @param vertexArray The vertex array, created by this context.
        /// @param vertexCount The number of vertices, 3 per triangle. More vertices than a vertex buffer of the array
        /// holds are refused with an engine error.
        virtual void Draw(const VertexArray& vertexArray, std::uint32_t vertexCount) = 0;

        /// @brief Draws the triangles of the index buffer of a vertex array, with the bound pipeline or shader, into the
        /// bound framebuffer or the window. A vertex array without an index buffer is refused with an engine error.
        /// @param vertexArray The vertex array, created by this context. Its indices must be below the number of
        /// vertices of its vertex buffers.
        virtual void DrawIndexed(const VertexArray& vertexArray) = 0;

    protected:
        /// @brief Destroys the objects that the context made for itself, such as the white texture and the queries of the
        /// profiler. The context of each
        /// graphics API calls it at the start of its destructor, while the API can still free them.
        void ReleaseResources();

        /// @brief Returns the counts of the frame being rendered, which the commands of the graphics API add to.
        RenderStatistics& GetCurrentStatistics() { return m_CurrentStatistics; }

        /// @brief Called by EndFrame(), for the graphics API to end its frame.
        virtual void OnEndFrame() = 0;

    private:
        Scope<Texture2D> m_WhiteTexture;
        Scope<GpuProfiler> m_Profiler;
        RenderStatistics m_CurrentStatistics;
        RenderStatistics m_FrameStatistics;
        std::uint64_t m_FrameNumber = 0;
    };
}

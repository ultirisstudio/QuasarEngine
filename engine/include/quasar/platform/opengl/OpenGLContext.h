#pragma once

/// \file
/// \brief The OpenGL 4.5 core context of a window.
/// \author Kurogenshi
/// \date 2026-11-10

#include <quasar/core/Base.h>
#include <quasar/renderer/GraphicsAPI.h>
#include <quasar/renderer/GraphicsContext.h>

#include <glm/vec2.hpp>

#include <cstddef>
#include <cstdint>
#include <span>

struct SDL_Window;
struct SDL_GLContextState;

namespace Quasar
{
    class OpenGLDevice;

    /// @brief OpenGL 4.5 core context of a window, created with SDL. The OpenGL functions are loaded with glad.
    ///
    /// A window created with GraphicsAPI::OpenGL owns one, which Window::GetGraphicsContext() returns.
    class OpenGLContext final : public GraphicsContext
    {
    public:
        /// @brief Sets the attributes of the next OpenGL contexts that SDL creates: OpenGL 4.5 core, double buffered,
        /// with a depth and a stencil buffer. Call it before the window is created.
        static void SetAttributes();

        /// @brief Creates the context of a window, makes it current and loads the OpenGL functions. Depths go from 0 to
        /// 1, as in Vulkan, so that both APIs use the same projection matrices.
        /// @param window The SDL window, created with SDL_WINDOW_OPENGL.
        /// @return The context, or nullptr if the driver does not provide OpenGL 4.5 core. The reason is logged as an
        /// engine error.
        static Scope<OpenGLContext> Create(SDL_Window* window);

        ~OpenGLContext() override;

        OpenGLContext(const OpenGLContext&) = delete;
        OpenGLContext& operator=(const OpenGLContext&) = delete;
        OpenGLContext(OpenGLContext&&) = delete;
        OpenGLContext& operator=(OpenGLContext&&) = delete;

        /// @brief Returns GraphicsAPI::OpenGL.
        GraphicsAPI GetAPI() const override { return GraphicsAPI::OpenGL; }

        /// @brief Shows the image drawn since the last call in the window.
        void SwapBuffers() override;

        /// @brief Enables or disables vertical synchronization.
        void SetVSync(bool enabled) override;

        /// @brief Makes the context current, then creates an OpenGL framebuffer. See GraphicsContext::CreateFramebuffer().
        [[nodiscard]] Scope<Framebuffer> CreateFramebuffer(const FramebufferSpecification& specification) override;

        /// @brief Makes the context current, then compiles an OpenGL shader. See GraphicsContext::CreateShader().
        [[nodiscard]] Scope<Shader> CreateShader(const ShaderSpecification& specification) override;

        /// @brief Creates an OpenGL pipeline. See GraphicsContext::CreatePipeline().
        [[nodiscard]] Scope<Pipeline> CreatePipeline(const PipelineSpecification& specification) override;

        /// @brief Makes the context current, then creates an OpenGL vertex buffer. See GraphicsContext::CreateVertexBuffer().
        [[nodiscard]] Scope<VertexBuffer> CreateVertexBuffer(std::span<const std::byte> vertices, const VertexLayout& layout) override;

        /// @brief Makes the context current, then creates an OpenGL index buffer. See GraphicsContext::CreateIndexBuffer().
        [[nodiscard]] Scope<IndexBuffer> CreateIndexBuffer(std::span<const std::uint32_t> indices) override;

        /// @brief Makes the context current, then creates an OpenGL uniform buffer. See
        /// GraphicsContext::CreateUniformBuffer().
        [[nodiscard]] Scope<UniformBuffer> CreateUniformBuffer(std::size_t size, std::uint32_t binding) override;

        /// @brief Makes the context current, then creates a frame uniform buffer. See
        /// GraphicsContext::CreateFrameUniformBuffer().
        [[nodiscard]] Scope<FrameUniformBuffer> CreateFrameUniformBuffer(std::size_t partSize, std::uint32_t binding) override;

        /// @brief Makes the context current, then creates a frame storage buffer. See
        /// GraphicsContext::CreateFrameStorageBuffer().
        [[nodiscard]] Scope<FrameStorageBuffer> CreateFrameStorageBuffer(std::size_t elementSize, std::uint32_t binding) override;

        /// @brief Makes the context current, then creates an OpenGL texture. See GraphicsContext::CreateTexture2D().
        [[nodiscard]] Scope<Texture2D> CreateTexture2D(const TextureSpecification& specification, std::span<const std::byte> pixels) override;

        /// @brief Makes the context current, then creates an OpenGL vertex array. See GraphicsContext::CreateVertexArray().
        [[nodiscard]] Scope<VertexArray> CreateVertexArray() override;

        /// @brief Makes the context current, then creates an OpenGL timestamp query. See
        /// GraphicsContext::CreateTimestampQuery().
        [[nodiscard]] Scope<TimestampQuery> CreateTimestampQuery() override;

        /// @brief Forgets what is bound and set. See GraphicsContext::InvalidateState().
        void InvalidateState() override;

        /// @brief Makes the context current, then makes the next draws go into the window. See
        /// GraphicsContext::BindWindow().
        void BindWindow() override;

        /// @brief Returns the size of the drawing area of the window, in pixels. See GraphicsContext::GetWindowSize().
        glm::uvec2 GetWindowSize() const override;

        /// @brief Returns the most samples per pixel of a framebuffer, read when the context is created. See
        /// GraphicsContext::GetMaxSamples().
        std::uint32_t GetMaxSamples() const override;

        /// @brief Returns the most anisotropy of a texture, read when the context is created. See
        /// GraphicsContext::GetMaxAnisotropy().
        float GetMaxAnisotropy() const override;

        /// @brief Makes the context current, then fills the window. See GraphicsContext::ClearWindow().
        void ClearWindow(glm::vec4 color, float depth, std::uint8_t stencil) override;

        /// @brief Draws triangles with the current OpenGL context, which the functions that create objects, BindWindow()
        /// and ClearWindow() make this one. See GraphicsContext::Draw().
        void Draw(const VertexArray& vertexArray, std::uint32_t vertexCount) override;

        /// @brief Draws indexed triangles with the current OpenGL context, as Draw() does. See
        /// GraphicsContext::DrawIndexed().
        void DrawIndexed(const VertexArray& vertexArray) override;

        /// @brief Draws instances of indexed triangles with the current OpenGL context, as Draw() does. See
        /// GraphicsContext::DrawInstances().
        void DrawInstances(const VertexArray& vertexArray, std::uint32_t instanceCount, std::uint32_t firstInstance) override;

        /// @brief Returns the SDL OpenGL context, for the code that needs to call SDL or OpenGL directly.
        SDL_GLContextState* GetNativeContext() const { return m_Context; }

    private:
        OpenGLContext(SDL_Window* window, SDL_GLContextState* context);

        void OnEndFrame() override;

        SDL_Window* m_Window = nullptr;
        SDL_GLContextState* m_Context = nullptr;
        Scope<OpenGLDevice> m_Device;
    };
}

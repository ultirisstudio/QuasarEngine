#pragma once

/// \file
/// \brief Draws one triangle that covers a whole framebuffer, the base of the image effects.
/// \author Kurogenshi
/// \date 2026-11-21

#include <quasar/core/Base.h>
#include <quasar/renderer/Framebuffer.h>
#include <quasar/renderer/GraphicsContext.h>
#include <quasar/renderer/Pipeline.h>
#include <quasar/renderer/VertexArray.h>

namespace Quasar
{
    /// @brief Draws one triangle that covers a whole target, so that a fragment shader runs once per pixel, for example
    /// to process the image of a previous pass.
    ///
    /// The triangle has no vertex buffer: the vertex stage places its 3 vertices from `gl_VertexID`. The engine shaders
    /// include this stage with:
    /// ```glsl
    /// #type vertex
    /// #version 450 core
    /// #include "include/FullscreenTriangle.glsl"
    /// ```
    /// The pipeline should have no depth test, no depth write and no culling.
    class FullscreenPass
    {
    public:
        /// @brief Creates a fullscreen pass that draws with a context, which must outlive it.
        /// @return The pass, or nullptr if its vertex array cannot be created. The reason is logged as an engine error.
        static Scope<FullscreenPass> Create(GraphicsContext& context);

        ~FullscreenPass() = default;

        FullscreenPass(const FullscreenPass&) = delete;
        FullscreenPass& operator=(const FullscreenPass&) = delete;
        FullscreenPass(FullscreenPass&&) = delete;
        FullscreenPass& operator=(FullscreenPass&&) = delete;

        /// @brief Draws the triangle with a pipeline over a whole target.
        /// @param pipeline The pipeline, whose shader and textures are already set.
        /// @param target The framebuffer to draw into, or nullptr for the window. The window is bound again afterwards.
        void Draw(Pipeline& pipeline, Framebuffer* target);

        /// @brief Returns the context that the pass draws with.
        GraphicsContext& GetContext() const { return *m_Context; }

    private:
        FullscreenPass(GraphicsContext& context, Scope<VertexArray> emptyVertexArray);

        GraphicsContext* m_Context;
        Scope<VertexArray> m_EmptyVertexArray;
    };
}

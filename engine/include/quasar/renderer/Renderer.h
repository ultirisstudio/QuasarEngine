#pragma once

/// \file
/// \brief The renderer: draws meshes with their materials, seen from a camera and lit by the lights of a scene.
/// \author Kurogenshi
/// \date 2026-11-18

#include <quasar/core/Base.h>
#include <quasar/renderer/Buffer.h>
#include <quasar/renderer/Camera.h>
#include <quasar/renderer/Framebuffer.h>
#include <quasar/renderer/GraphicsContext.h>
#include <quasar/renderer/Material.h>
#include <quasar/renderer/Mesh.h>

#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace Quasar
{
    /// @brief A light that comes from the same direction everywhere, such as the sun.
    struct DirectionalLight
    {
        /// The direction in which the light goes, which does not need a length of 1. A zero direction goes down.
        glm::vec3 Direction{-0.4f, -1.0f, -0.6f};
        glm::vec3 Color{1.0f};  ///< The color of the light, in sRGB as in a color picker.
        float Intensity = 1.0f; ///< The factor of the color.
        float Ambient = 0.1f;   ///< The part of the light that reaches every surface, even the ones that face away.
    };

    /// @brief A light that shines from a point in every direction, such as a bulb.
    ///
    /// It lights a surface by its intensity divided by the square of the distance, as a real light does, and fades
    /// smoothly to nothing at its range, so that it lights only the surfaces near it.
    struct PointLight
    {
        glm::vec3 Position{0.0f}; ///< The position, in the world.
        glm::vec3 Color{1.0f};    ///< The color of the light, in sRGB as in a color picker.
        float Intensity = 1.0f;   ///< The factor of the color, which reaches 1 at a distance of 1.
        float Range = 10.0f;      ///< The distance at which the light ends.
    };

    /// @brief A light that shines from a point in a cone, such as a torch, and fades with the distance like a PointLight.
    ///
    /// The light is full inside the inner angle, and fades smoothly to nothing at the outer angle. Both angles go from the
    /// axis of the cone to its side, and an inner angle wider than the outer one gives a sharp edge at the outer one.
    struct SpotLight
    {
        glm::vec3 Position{0.0f};               ///< The position, in the world.
        glm::vec3 Direction{0.0f, 0.0f, -1.0f}; ///< The axis of the cone, which does not need a length of 1.
        glm::vec3 Color{1.0f};                  ///< The color of the light, in sRGB as in a color picker.
        float Intensity = 1.0f;                 ///< The factor of the color, which reaches 1 at a distance of 1.
        float Range = 10.0f;                    ///< The distance at which the light ends.
        float InnerAngle = glm::radians(20.0f); ///< The angle of the full light, in radians.
        float OuterAngle = glm::radians(30.0f); ///< The angle where the light ends, in radians, below 180 degrees.
    };

    /// @brief The lights of a scene: at most one directional light, and point lights and spot lights.
    struct SceneLights
    {
        std::optional<DirectionalLight> Directional = DirectionalLight{}; ///< The directional light, if the scene has one.
        std::vector<PointLight> Points{};                                 ///< The point lights.
        std::vector<SpotLight> Spots{};                                   ///< The spot lights.
    };

    /// @brief Draws meshes with their materials, seen from a camera and lit by the lights of a scene, into a framebuffer
    /// or the window.
    ///
    /// The shaders read the camera, the object and the lights from these uniform blocks, and the materials use the
    /// binding 1:
    /// ```glsl
    /// layout(std140, binding = 0) uniform Camera
    /// {
    ///     mat4 u_ViewProjection;
    ///     mat4 u_View;
    ///     mat4 u_Projection;
    ///     vec4 u_CameraPosition;
    /// };
    /// layout(std140, binding = 2) uniform Object
    /// {
    ///     mat4 u_Model;
    /// };
    /// struct PointLight
    /// {
    ///     vec4 PositionRange; // The position in xyz and the range in w.
    ///     vec4 Color;         // The linear color multiplied by the intensity in rgb.
    /// };
    /// struct SpotLight
    /// {
    ///     vec4 PositionRange;  // The position in xyz and the range in w.
    ///     vec4 DirectionOuter; // The axis, with a length of 1, in xyz and the cosine of the outer angle in w.
    ///     vec4 ColorInner;     // The linear color multiplied by the intensity in rgb and the cosine of the inner angle in w.
    /// };
    /// layout(std140, binding = 3) uniform Lights
    /// {
    ///     vec3 u_DirectionalDirection; // The direction in which the light goes, with a length of 1.
    ///     float u_DirectionalAmbient;
    ///     vec3 u_DirectionalColor;     // The linear color multiplied by the intensity, black without a directional light.
    ///     int u_PointLightCount;
    ///     int u_SpotLightCount;
    ///     PointLight u_PointLights[32];
    ///     SpotLight u_SpotLights[32];
    /// };
    /// ```
    /// A scene uses at most MaxPointLights point lights and MaxSpotLights spot lights. The next ones are not used, and an
    /// engine warning is logged once until the scene has few enough lights again.
    /// A scene is drawn between BeginScene() and EndScene(), and each Submit() draws one mesh at once. The target is not
    /// cleared, so that several scenes can be drawn into it.
    ///
    /// The shaders compute and write linear colors, which can go above 1, for example into a framebuffer of format
    /// FramebufferFormat::Rgba16F. The SceneRenderer turns them into the colors of the screen at the end.
    class Renderer
    {
    public:
        static constexpr std::uint32_t CameraBinding = 0; ///< The binding of the `Camera` block.
        static constexpr std::uint32_t ObjectBinding = 2; ///< The binding of the `Object` block.
        static constexpr std::uint32_t LightBinding = 3;  ///< The binding of the `Lights` block.
        static constexpr std::size_t MaxPointLights = 32; ///< The size of the `u_PointLights` array of the shaders.
        static constexpr std::size_t MaxSpotLights = 32;  ///< The size of the `u_SpotLights` array of the shaders.

        /// @brief Creates a renderer that draws with a context.
        /// @return The renderer, or nullptr if the context cannot create its uniform buffers. The reason is logged as
        /// an engine error.
        static Scope<Renderer> Create(GraphicsContext& context);

        ~Renderer() = default;

        Renderer(const Renderer&) = delete;
        Renderer& operator=(const Renderer&) = delete;
        Renderer(Renderer&&) = delete;
        Renderer& operator=(Renderer&&) = delete;

        /// @brief Starts a scene seen from a camera.
        /// @param camera The camera, whose viewport size should be the size of the target.
        /// @param target The framebuffer to draw into, or nullptr for the window. It must stay alive until EndScene().
        /// @param lights The lights of the scene.
        ///
        /// A scene started before the previous one ended is refused with an engine error.
        void BeginScene(const Camera& camera, Framebuffer* target = nullptr, const SceneLights& lights = {});

        /// @brief Draws a mesh with a material.
        /// @param mesh The mesh.
        /// @param material The material, which must not use the bindings of the renderer.
        /// @param transform The matrix that places the mesh in the world.
        ///
        /// A mesh submitted outside of a scene, or with a material on a binding of the renderer, is not drawn and logs
        /// an engine error.
        void Submit(const Mesh& mesh, Material& material, const glm::mat4& transform);

        /// @brief Ends the scene, and makes the next draws go into the window. Ending without a scene is refused with
        /// an engine error.
        void EndScene();

        /// @brief Returns the context that the renderer draws with.
        GraphicsContext& GetContext() const { return *m_Context; }

    private:
        Renderer(GraphicsContext& context, Scope<UniformBuffer> camera, Scope<UniformBuffer> object, Scope<UniformBuffer> light);

        GraphicsContext* m_Context;
        Scope<UniformBuffer> m_CameraBuffer;
        Scope<UniformBuffer> m_ObjectBuffer;
        Scope<UniformBuffer> m_LightBuffer;
        bool m_ReportedTooManyLights = false;
        Framebuffer* m_Target = nullptr;
        bool m_InScene = false;
        std::vector<const Material*> m_ReportedMaterials;
    };
}

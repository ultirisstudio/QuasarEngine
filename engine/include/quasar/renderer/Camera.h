#pragma once

/// \file
/// \brief Cameras: the position, the orientation and the projection from which a scene is drawn.
/// \author Kurogenshi
/// \date 2026-11-18

#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>

#include <cstdint>

namespace Quasar
{
    /// @brief Settings of a perspective projection, in which far objects look smaller.
    struct PerspectiveProjection
    {
        float VerticalFov = glm::radians(45.0f); ///< The vertical field of view, in radians, between 0 and pi.
        float Near = 0.1f;                       ///< The distance of the near plane, above 0.
        float Far = 1000.0f;                     ///< The distance of the far plane, above Near.
    };

    /// @brief Settings of an orthographic projection, in which objects keep their size whatever their distance, for
    /// example for a 2D game, a view of the editor from a side, or the shadows of the sun.
    struct OrthographicProjection
    {
        float Height = 10.0f; ///< The height of the visible area, above 0. Its width is the height times the aspect ratio.
        float Near = 0.0f;    ///< The distance of the near plane, which can be 0 or below to see behind the camera.
        float Far = 100.0f;   ///< The distance of the far plane, above Near.
    };

    /// @brief The kind of projection of a camera.
    enum class ProjectionType : std::uint8_t
    {
        Perspective, ///< See PerspectiveProjection.
        Orthographic ///< See OrthographicProjection.
    };

    /// @brief The point of view from which a scene is drawn, for a camera of the game or of the editor.
    ///
    /// The camera looks toward its -Z axis, with +Y up and +X to the right, as in OpenGL and glTF. Its projection gives
    /// depths from 0 on the near plane to 1 on the far plane, with every graphics API. The matrices are computed when a
    /// setting changes, so the getters only return them.
    class Camera
    {
    public:
        /// @brief Creates a camera at the origin, looking toward -Z, with the default PerspectiveProjection and an
        /// aspect ratio of 1.
        Camera();

        /// @brief Makes the camera use a perspective projection. A field of view outside of 0 to pi, a near plane at 0 or
        /// below, or a far plane not above the near plane is refused with an engine error, and the projection is not
        /// changed.
        void SetPerspective(const PerspectiveProjection& projection);

        /// @brief Returns the settings of the perspective projection, kept while the camera is orthographic.
        const PerspectiveProjection& GetPerspective() const { return m_Perspective; }

        /// @brief Makes the camera use an orthographic projection. A height at 0 or below, or a far plane not above the
        /// near plane is refused with an engine error, and the projection is not changed.
        void SetOrthographic(const OrthographicProjection& projection);

        /// @brief Returns the settings of the orthographic projection, kept while the camera is perspective.
        const OrthographicProjection& GetOrthographic() const { return m_Orthographic; }

        /// @brief Returns the kind of projection that the camera uses.
        ProjectionType GetProjectionType() const { return m_ProjectionType; }

        /// @brief Sets the size of the image that the camera draws, which gives its aspect ratio. A size of 0, for
        /// example while a window is minimized, is ignored.
        void SetViewportSize(std::uint32_t width, std::uint32_t height);

        /// @brief Returns the width of the image divided by its height.
        float GetAspectRatio() const { return m_AspectRatio; }

        /// @brief Moves the camera.
        void SetPosition(const glm::vec3& position);

        /// @brief Returns the position of the camera.
        const glm::vec3& GetPosition() const { return m_Position; }

        /// @brief Turns the camera. The orientation is normalized, and a zero quaternion is refused with an engine error.
        void SetOrientation(const glm::quat& orientation);

        /// @brief Returns the orientation of the camera, as a unit quaternion.
        const glm::quat& GetOrientation() const { return m_Orientation; }

        /// @brief Turns the camera toward a point, with its top toward an up direction.
        ///
        /// A target at the position of the camera, or in the up direction from it, gives no orientation: it is refused
        /// with an engine error, and the camera is not turned.
        void LookAt(const glm::vec3& target, const glm::vec3& up = {0.0f, 1.0f, 0.0f});

        /// @brief Returns the direction in which the camera looks.
        glm::vec3 GetForward() const;

        /// @brief Returns the direction to the right of the image.
        glm::vec3 GetRight() const;

        /// @brief Returns the direction to the top of the image.
        glm::vec3 GetUp() const;

        /// @brief Returns the matrix that moves the points of the world into the space of the camera.
        const glm::mat4& GetView() const { return m_View; }

        /// @brief Returns the matrix that projects the points of the space of the camera onto the image.
        const glm::mat4& GetProjection() const { return m_Projection; }

        /// @brief Returns GetProjection() * GetView(), which projects the points of the world onto the image.
        const glm::mat4& GetViewProjection() const { return m_ViewProjection; }

    private:
        void UpdateView();
        void UpdateProjection();

        PerspectiveProjection m_Perspective;
        OrthographicProjection m_Orthographic;
        ProjectionType m_ProjectionType = ProjectionType::Perspective;
        float m_AspectRatio = 1.0f;
        glm::vec3 m_Position{0.0f};
        glm::quat m_Orientation;

        glm::mat4 m_View{1.0f};
        glm::mat4 m_Projection{1.0f};
        glm::mat4 m_ViewProjection{1.0f};
    };
}

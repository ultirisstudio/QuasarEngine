#include <quasar/renderer/Camera.h>

#include <quasar/log/Log.h>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <cstdint>

namespace Quasar
{
    namespace
    {
        constexpr float Epsilon = 1e-6f;
    }

    Camera::Camera() : m_Orientation(glm::identity<glm::quat>())
    {
        UpdateView();
        UpdateProjection();
    }

    void Camera::SetPerspective(const PerspectiveProjection& projection)
    {
        if (!(projection.VerticalFov > 0.0f) || !(projection.VerticalFov < glm::pi<float>()))
        {
            QUASAR_ENGINE_ERROR("Cannot use a vertical field of view of {} radians: it must be between 0 and pi", projection.VerticalFov);
            return;
        }
        if (!(projection.Near > 0.0f) || !(projection.Far > projection.Near))
        {
            QUASAR_ENGINE_ERROR("Cannot use a near plane at {} and a far plane at {}: they must be above 0, and the far plane above the near plane",
                                projection.Near, projection.Far);
            return;
        }
        m_Perspective = projection;
        UpdateProjection();
    }

    void Camera::SetViewportSize(std::uint32_t width, std::uint32_t height)
    {
        if (width == 0 || height == 0)
        {
            return;
        }
        m_AspectRatio = static_cast<float>(width) / static_cast<float>(height);
        UpdateProjection();
    }

    void Camera::SetPosition(const glm::vec3& position)
    {
        m_Position = position;
        UpdateView();
    }

    void Camera::SetOrientation(const glm::quat& orientation)
    {
        if (!(glm::dot(orientation, orientation) > Epsilon))
        {
            QUASAR_ENGINE_ERROR("Cannot turn a camera with a zero quaternion");
            return;
        }
        m_Orientation = glm::normalize(orientation);
        UpdateView();
    }

    void Camera::LookAt(const glm::vec3& target, const glm::vec3& up)
    {
        const glm::vec3 direction = target - m_Position;
        if (!(glm::dot(direction, direction) > Epsilon))
        {
            QUASAR_ENGINE_ERROR("Cannot turn a camera toward its own position");
            return;
        }
        const glm::vec3 forward = glm::normalize(direction);
        const glm::vec3 side = glm::cross(forward, up);
        if (!(glm::dot(side, side) > Epsilon))
        {
            QUASAR_ENGINE_ERROR("Cannot turn a camera toward a target in its up direction");
            return;
        }
        m_Orientation = glm::normalize(glm::quatLookAtRH(forward, glm::normalize(up)));
        UpdateView();
    }

    glm::vec3 Camera::GetForward() const
    {
        return m_Orientation * glm::vec3(0.0f, 0.0f, -1.0f);
    }

    glm::vec3 Camera::GetRight() const
    {
        return m_Orientation * glm::vec3(1.0f, 0.0f, 0.0f);
    }

    glm::vec3 Camera::GetUp() const
    {
        return m_Orientation * glm::vec3(0.0f, 1.0f, 0.0f);
    }

    void Camera::UpdateView()
    {
        m_View = glm::mat4_cast(glm::conjugate(m_Orientation)) * glm::translate(glm::mat4(1.0f), -m_Position);
        m_ViewProjection = m_Projection * m_View;
    }

    void Camera::UpdateProjection()
    {
        m_Projection = glm::perspectiveRH_ZO(m_Perspective.VerticalFov, m_AspectRatio, m_Perspective.Near, m_Perspective.Far);
        m_ViewProjection = m_Projection * m_View;
    }
}

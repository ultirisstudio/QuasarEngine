#include "helpers/LogHelpers.h"
#include "helpers/MathHelpers.h"

#include <quasar/log/MemoryLogSink.h>
#include <quasar/renderer/Camera.h>

#include <doctest/doctest.h>

#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <optional>
#include <vector>

using Quasar::Camera;
using Quasar::PerspectiveProjection;
using TestHelpers::IsNear;

namespace
{
    // The position on the image, from -1 to 1, and the depth, or nothing behind the camera
    std::optional<glm::vec3> Project(const Camera& camera, const glm::vec3& point)
    {
        const glm::vec4 clip = camera.GetViewProjection() * glm::vec4(point, 1.0f);
        if (clip.w <= 0.0f)
        {
            return std::nullopt;
        }
        return glm::vec3(clip) / clip.w;
    }

    bool IsVisible(const Camera& camera, const glm::vec3& point)
    {
        const std::optional<glm::vec3> projected = Project(camera, point);
        return projected && std::abs(projected->x) <= 1.0f && std::abs(projected->y) <= 1.0f && projected->z >= 0.0f && projected->z <= 1.0f;
    }

    bool IsCentered(const Camera& camera, const glm::vec3& point)
    {
        const std::optional<glm::vec3> projected = Project(camera, point);
        return projected && std::abs(projected->x) < 1e-4f && std::abs(projected->y) < 1e-4f;
    }
}

TEST_CASE("A camera sees the points in front of it, with depths from 0 on the near plane to 1 on the far plane")
{
    Camera camera;
    camera.SetPerspective({.VerticalFov = glm::radians(90.0f), .Near = 1.0f, .Far = 100.0f});

    CHECK(IsVisible(camera, {0.0f, 0.0f, -10.0f}));
    CHECK_FALSE(IsVisible(camera, {0.0f, 0.0f, 10.0f}));
    CHECK_FALSE(IsVisible(camera, {0.0f, 0.0f, -0.5f}));
    CHECK_FALSE(IsVisible(camera, {0.0f, 0.0f, -101.0f}));
    CHECK(IsNear(Project(camera, {0.0f, 0.0f, -1.0f})->z, 0.0f));
    CHECK(IsNear(Project(camera, {0.0f, 0.0f, -100.0f})->z, 1.0f));
    CHECK(Project(camera, {0.0f, 0.0f, -10.0f})->z < Project(camera, {0.0f, 0.0f, -20.0f})->z);
}

TEST_CASE("The vertical field of view reaches the top of the image, and the aspect ratio widens it")
{
    Camera camera;
    camera.SetPerspective({.VerticalFov = glm::radians(90.0f), .Near = 0.1f, .Far = 100.0f});

    CHECK(IsNear(Project(camera, {0.0f, 10.0f, -10.0f})->y, 1.0f));
    CHECK(IsNear(Project(camera, {10.0f, 0.0f, -10.0f})->x, 1.0f));
    CHECK(IsNear(Project(camera, {-5.0f, 0.0f, -10.0f})->x, -0.5f));

    camera.SetViewportSize(1920, 960);
    CHECK(camera.GetAspectRatio() == 2.0f);
    CHECK(IsNear(Project(camera, {20.0f, 0.0f, -10.0f})->x, 1.0f));
    CHECK(IsNear(Project(camera, {0.0f, 10.0f, -10.0f})->y, 1.0f));

    camera.SetViewportSize(0, 960);
    camera.SetViewportSize(1920, 0);
    CHECK(camera.GetAspectRatio() == 2.0f);
}

TEST_CASE("The view follows the position and the orientation of the camera")
{
    Camera camera;
    camera.SetPosition({0.0f, 0.0f, 10.0f});
    CHECK(IsCentered(camera, {0.0f, 0.0f, 0.0f}));
    CHECK(IsVisible(camera, {0.0f, 0.0f, 0.0f}));

    camera.SetOrientation(glm::angleAxis(glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)));
    CHECK(IsNear(camera.GetForward(), {-1.0f, 0.0f, 0.0f}));
    CHECK(IsNear(camera.GetRight(), {0.0f, 0.0f, -1.0f}));
    CHECK(IsNear(camera.GetUp(), {0.0f, 1.0f, 0.0f}));
    CHECK_FALSE(IsVisible(camera, {0.0f, 0.0f, 0.0f}));
    CHECK(IsCentered(camera, {-10.0f, 0.0f, 10.0f}));
    CHECK(Project(camera, {-10.0f, 0.0f, 9.0f})->x > 0.0f);

    const glm::vec4 point(3.0f, -2.0f, 1.0f, 1.0f);
    CHECK(IsNear(glm::vec3(camera.GetViewProjection() * point), glm::vec3(camera.GetProjection() * (camera.GetView() * point))));
}

TEST_CASE("LookAt turns the camera toward a point, with its top toward the up direction")
{
    const TestHelpers::ScopedLog log;
    Camera camera;
    camera.SetPosition({10.0f, 0.0f, 0.0f});

    camera.LookAt({0.0f, 0.0f, 0.0f});
    CHECK(IsNear(camera.GetForward(), {-1.0f, 0.0f, 0.0f}));
    CHECK(IsNear(camera.GetUp(), {0.0f, 1.0f, 0.0f}));
    CHECK(IsCentered(camera, {0.0f, 0.0f, 0.0f}));
    CHECK(Project(camera, {0.0f, 1.0f, 0.0f})->y > 0.0f);

    camera.LookAt({10.0f, 0.0f, -5.0f}, {1.0f, 0.0f, 0.0f});
    CHECK(IsNear(camera.GetForward(), {0.0f, 0.0f, -1.0f}));
    CHECK(IsNear(camera.GetUp(), {1.0f, 0.0f, 0.0f}));
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
}

TEST_CASE("A camera keeps its orientation when it cannot turn, and its projection when the settings are refused")
{
    Camera camera;
    camera.SetPosition({0.0f, 5.0f, 0.0f});
    camera.LookAt({0.0f, 5.0f, -10.0f});
    const glm::quat orientation = camera.GetOrientation();
    const PerspectiveProjection projection{.VerticalFov = glm::radians(60.0f), .Near = 0.5f, .Far = 50.0f};
    camera.SetPerspective(projection);
    const glm::vec3 projected = *Project(camera, {1.0f, 6.0f, -10.0f});

    for (const glm::vec3& target : {glm::vec3(0.0f, 5.0f, 0.0f), glm::vec3(0.0f, 20.0f, 0.0f), glm::vec3(0.0f, -20.0f, 0.0f)})
    {
        const TestHelpers::ScopedLog log;
        camera.LookAt(target);
        CHECK(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
    }
    {
        const TestHelpers::ScopedLog log;
        camera.SetOrientation(glm::quat(0.0f, 0.0f, 0.0f, 0.0f));
        CHECK(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
    }
    CHECK(camera.GetOrientation() == orientation);

    for (const PerspectiveProjection& refused :
         {PerspectiveProjection{.VerticalFov = 0.0f}, PerspectiveProjection{.VerticalFov = glm::radians(180.0f)}, PerspectiveProjection{.Near = 0.0f},
          PerspectiveProjection{.Near = 10.0f, .Far = 10.0f}, PerspectiveProjection{.VerticalFov = std::nanf("")}})
    {
        const TestHelpers::ScopedLog log;
        camera.SetPerspective(refused);
        CHECK(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
    }
    CHECK(IsNear(*Project(camera, {1.0f, 6.0f, -10.0f}), projected));
}

TEST_CASE("A camera normalizes its orientation")
{
    Camera camera;
    const glm::quat turn = glm::angleAxis(glm::radians(30.0f), glm::normalize(glm::vec3(1.0f, 1.0f, 0.0f)));

    camera.SetOrientation(turn * 3.0f);

    CHECK(IsNear(glm::length(camera.GetOrientation()), 1.0f));
    CHECK(IsNear(camera.GetForward(), turn * glm::vec3(0.0f, 0.0f, -1.0f)));
}

TEST_CASE("An orthographic camera keeps the size of the objects whatever their distance, and its width follows the aspect ratio")
{
    const TestHelpers::ScopedLog log;
    Camera camera;
    camera.SetOrthographic({.Height = 4.0f, .Near = -1.0f, .Far = 9.0f});
    camera.SetViewportSize(200, 100);

    CHECK(camera.GetProjectionType() == Quasar::ProjectionType::Orthographic);
    CHECK(camera.GetOrthographic().Height == 4.0f);
    const std::optional<glm::vec3> near = Project(camera, {4.0f, 2.0f, 1.0f});
    const std::optional<glm::vec3> far = Project(camera, {4.0f, 2.0f, -9.0f});
    REQUIRE(near.has_value());
    REQUIRE(far.has_value());
    CHECK(glm::distance(*near, glm::vec3(1.0f, 1.0f, 0.0f)) < 1e-5f);
    CHECK(glm::distance(*far, glm::vec3(1.0f, 1.0f, 1.0f)) < 1e-5f);
    CHECK_FALSE(IsVisible(camera, {0.0f, 0.0f, -10.0f}));

    camera.SetPerspective({.VerticalFov = glm::radians(90.0f), .Near = 0.5f, .Far = 9.0f});
    CHECK(camera.GetProjectionType() == Quasar::ProjectionType::Perspective);
    CHECK(camera.GetOrthographic().Height == 4.0f);
    CHECK_FALSE(IsVisible(camera, {0.0f, 0.0f, 1.0f}));
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));
}

TEST_CASE("An orthographic projection without height, or whose far plane is not beyond its near plane, is refused")
{
    const TestHelpers::ScopedLog log;
    Camera camera;
    const glm::mat4 projection = camera.GetProjection();

    camera.SetOrthographic({.Height = 0.0f});
    camera.SetOrthographic({.Height = 1.0f, .Near = 2.0f, .Far = 2.0f});

    CHECK(camera.GetProjectionType() == Quasar::ProjectionType::Perspective);
    CHECK(camera.GetProjection() == projection);
    const std::vector<Quasar::LogMessage> messages = Quasar::Log::GetMemorySink()->GetMessages();
    CHECK(std::ranges::count_if(messages, [](const Quasar::LogMessage& message) { return message.Level == Quasar::LogLevel::Error; }) == 2);
}

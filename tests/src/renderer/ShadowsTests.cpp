#include "helpers/LogHelpers.h"

#include <quasar/renderer/Camera.h>
#include <quasar/renderer/Shadows.h>

#include <doctest/doctest.h>

#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <cmath>

using Quasar::Camera;
using Quasar::ComputeShadowCamera;
using Quasar::ShadowCamera;

namespace
{
    bool IsNear(const glm::vec3& value, const glm::vec3& expected, float tolerance = 1e-4f)
    {
        return glm::distance(value, expected) < tolerance;
    }

    Camera CreateViewCamera()
    {
        Camera camera;
        camera.SetViewportSize(16, 9);
        camera.SetPerspective({.VerticalFov = glm::radians(60.0f), .Near = 0.1f, .Far = 100.0f});
        camera.SetPosition({1.0f, 2.0f, 3.0f});
        return camera;
    }

    glm::vec3 Project(const ShadowCamera& shadow, const glm::vec3& point)
    {
        const glm::vec4 clip = shadow.View.GetViewProjection() * glm::vec4(point, 1.0f);
        return glm::vec3(clip) / clip.w;
    }

    bool IsOnMap(const ShadowCamera& shadow, const glm::vec3& point)
    {
        const glm::vec3 projected = Project(shadow, point);
        return std::abs(projected.x) <= 1.0f && std::abs(projected.y) <= 1.0f && projected.z >= 0.0f && projected.z <= 1.0f;
    }
}

TEST_CASE("The shadow camera looks along the light and covers the view of the camera up to the shadow distance")
{
    const Camera camera = CreateViewCamera();
    const ShadowCamera shadow = ComputeShadowCamera(camera, {0.0f, -2.0f, -2.0f}, {.Resolution = 512, .Distance = 10.0f});

    CHECK(shadow.View.GetProjectionType() == Quasar::ProjectionType::Orthographic);
    CHECK(IsNear(shadow.View.GetForward(), glm::normalize(glm::vec3(0.0f, -1.0f, -1.0f))));
    CHECK(shadow.Resolution == 512);
    CHECK(std::abs((shadow.TexelSize * 512.0f) - shadow.View.GetOrthographic().Height) < 1e-4f);
    CHECK(IsOnMap(shadow, camera.GetPosition() + (camera.GetForward() * 0.2f)));
    CHECK(IsOnMap(shadow, camera.GetPosition() + (camera.GetForward() * 9.5f)));
    CHECK_FALSE(IsOnMap(shadow, camera.GetPosition() + (camera.GetForward() * 40.0f)));
    CHECK_FALSE(IsOnMap(shadow, camera.GetPosition() - (camera.GetRight() * 40.0f)));
}

TEST_CASE("The shadow camera of an orthographic view covers it up to the shadow distance after its near plane")
{
    Camera camera = CreateViewCamera();
    camera.SetOrthographic({.Height = 2.0f, .Near = 5.0f, .Far = 1000.0f});
    const ShadowCamera shadow = ComputeShadowCamera(camera, {0.0f, -1.0f, 0.0f}, {.Distance = 3.0f});

    CHECK(IsOnMap(shadow, camera.GetPosition() + (camera.GetForward() * 7.5f)));
    CHECK_FALSE(IsOnMap(shadow, camera.GetPosition() + (camera.GetForward() * 20.0f)));
}

TEST_CASE("A shadow distance of 0 still covers a thin slice of the view")
{
    const TestHelpers::ScopedLog log;
    Camera camera = CreateViewCamera();
    const ShadowCamera perspective = ComputeShadowCamera(camera, {0.0f, -1.0f, 0.0f}, {.Distance = 0.0f});
    CHECK(IsOnMap(perspective, camera.GetPosition() + (camera.GetForward() * 0.15f)));
    CHECK_FALSE(IsOnMap(perspective, camera.GetPosition() + (camera.GetForward() * 5.0f)));

    camera.SetOrthographic({.Height = 2.0f, .Near = 5.0f, .Far = 1000.0f});
    const ShadowCamera orthographic = ComputeShadowCamera(camera, {0.0f, -1.0f, 0.0f}, {.Distance = 0.0f});
    CHECK(IsOnMap(orthographic, camera.GetPosition() + (camera.GetForward() * 5.005f)));
    CHECK_FALSE(IsOnMap(orthographic, camera.GetPosition() + (camera.GetForward() * 20.0f)));
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
}

TEST_CASE("A zero light direction shines down, and a resolution of 0 gives a map of one pixel")
{
    const ShadowCamera down = ComputeShadowCamera(CreateViewCamera(), glm::vec3(0.0f), {.Resolution = 0});
    CHECK(IsNear(down.View.GetForward(), {0.0f, -1.0f, 0.0f}));
    CHECK(IsNear(down.View.GetUp(), {0.0f, 0.0f, -1.0f}));
    CHECK(down.Resolution == 1);
    CHECK(std::abs(down.TexelSize - down.View.GetOrthographic().Height) < 1e-4f);
}

TEST_CASE("The shadow camera keeps its size when the camera turns, and moves by whole pixels when it moves")
{
    Camera camera = CreateViewCamera();
    const glm::vec3 direction(0.3f, -1.0f, 0.2f);
    const ShadowCamera first = ComputeShadowCamera(camera, direction, {.Resolution = 1024, .Distance = 20.0f});
    const glm::vec2 origin = glm::vec2(Project(first, glm::vec3(0.0f))) * 512.0f;

    for (const glm::vec3& move : {glm::vec3(0.0137f, 0.0f, 0.0f), glm::vec3(0.0f, 0.021f, 0.0091f), glm::vec3(1.37f, 0.4f, -2.2f)})
    {
        CAPTURE(move.x);
        camera.SetPosition(camera.GetPosition() + move);
        camera.SetOrientation(glm::angleAxis(0.31f, glm::vec3(0.0f, 1.0f, 0.0f)) * camera.GetOrientation());
        const ShadowCamera moved = ComputeShadowCamera(camera, direction, {.Resolution = 1024, .Distance = 20.0f});
        CHECK(std::abs(moved.TexelSize - first.TexelSize) < 1e-5f);
        const glm::vec2 shift = (glm::vec2(Project(moved, glm::vec3(0.0f))) * 512.0f) - origin;
        CHECK(std::abs(shift.x - std::round(shift.x)) < 0.01f);
        CHECK(std::abs(shift.y - std::round(shift.y)) < 0.01f);
    }
}

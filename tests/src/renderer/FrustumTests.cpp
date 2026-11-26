#include <quasar/renderer/BoundingBox.h>
#include <quasar/renderer/Camera.h>
#include <quasar/renderer/Frustum.h>

#include <doctest/doctest.h>

#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>

using Quasar::BoundingBox;
using Quasar::Camera;
using Quasar::Frustum;

namespace
{
    BoundingBox BoxAt(const glm::vec3& center, float halfSize = 0.5f)
    {
        return {.Min = center - glm::vec3(halfSize), .Max = center + glm::vec3(halfSize)};
    }

    Frustum CreatePerspectiveFrustum()
    {
        Camera camera;
        camera.SetViewportSize(1, 1);
        camera.SetPerspective({.VerticalFov = glm::radians(90.0f), .Near = 1.0f, .Far = 10.0f});
        camera.SetPosition({0.0f, 0.0f, 0.0f});
        return Frustum::FromViewProjection(camera.GetViewProjection());
    }
}

TEST_CASE("A perspective frustum sees the boxes in front of the camera, between its near and its far planes")
{
    const Frustum frustum = CreatePerspectiveFrustum();

    CHECK(frustum.Intersects(BoxAt({0.0f, 0.0f, -5.0f})));
    CHECK(frustum.Intersects(BoxAt({0.0f, 0.0f, -1.2f})));
    CHECK(frustum.Intersects(BoxAt({0.0f, 0.0f, -10.3f})));
    CHECK(frustum.Intersects(BoxAt({0.0f, 0.0f, 0.0f}, 100.0f)));

    CHECK_FALSE(frustum.Intersects(BoxAt({0.0f, 0.0f, 5.0f})));
    CHECK_FALSE(frustum.Intersects(BoxAt({0.0f, 0.0f, -0.2f}, 0.1f)));
    CHECK_FALSE(frustum.Intersects(BoxAt({0.0f, 0.0f, -11.0f})));
}

TEST_CASE("A perspective frustum does not see the boxes beside, above or below its view")
{
    const Frustum frustum = CreatePerspectiveFrustum();

    CHECK(frustum.Intersects(BoxAt({4.8f, 0.0f, -5.0f})));
    CHECK(frustum.Intersects(BoxAt({0.0f, -4.8f, -5.0f})));

    CHECK_FALSE(frustum.Intersects(BoxAt({7.0f, 0.0f, -5.0f})));
    CHECK_FALSE(frustum.Intersects(BoxAt({-7.0f, 0.0f, -5.0f})));
    CHECK_FALSE(frustum.Intersects(BoxAt({0.0f, 7.0f, -5.0f})));
    CHECK_FALSE(frustum.Intersects(BoxAt({0.0f, -7.0f, -5.0f})));
}

TEST_CASE("An orthographic frustum sees a box of its size, wherever the camera is and wherever it looks")
{
    Camera camera;
    camera.SetViewportSize(1, 1);
    camera.SetOrthographic({.Height = 4.0f, .Near = 0.0f, .Far = 20.0f});
    camera.SetPosition({10.0f, 0.0f, 0.0f});
    camera.LookAt({10.0f, -1.0f, 0.0f}, {0.0f, 0.0f, -1.0f});
    const Frustum frustum = Frustum::FromViewProjection(camera.GetViewProjection());

    CHECK(frustum.Intersects(BoxAt({10.0f, -10.0f, 0.0f})));
    CHECK(frustum.Intersects(BoxAt({11.9f, -19.9f, 1.9f})));

    CHECK_FALSE(frustum.Intersects(BoxAt({10.0f, 1.0f, 0.0f})));
    CHECK_FALSE(frustum.Intersects(BoxAt({10.0f, -21.0f, 0.0f})));
    CHECK_FALSE(frustum.Intersects(BoxAt({13.0f, -10.0f, 0.0f})));
    CHECK_FALSE(frustum.Intersects(BoxAt({10.0f, -10.0f, -3.0f})));
}

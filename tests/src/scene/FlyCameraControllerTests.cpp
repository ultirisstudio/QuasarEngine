#include <quasar/scene/FlyCameraController.h>
#include <quasar/scene/components/TransformComponent.h>
#include <quasar/time/Timestep.h>

#include <doctest/doctest.h>

#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

#include <cmath>

using Quasar::FlyCameraController;
using Quasar::Timestep;
using Quasar::TransformComponent;

namespace
{
    bool IsNear(const glm::vec3& value, const glm::vec3& expected)
    {
        return glm::length(value - expected) < 1e-4f;
    }

    bool IsNear(float value, float expected)
    {
        return std::abs(value - expected) < 1e-4f;
    }

    glm::vec3 Forward(const TransformComponent& transform)
    {
        return transform.Rotation * glm::vec3(0.0f, 0.0f, -1.0f);
    }

    glm::vec3 Right(const TransformComponent& transform)
    {
        return transform.Rotation * glm::vec3(1.0f, 0.0f, 0.0f);
    }
}

TEST_CASE("A fly camera turns by the look input, to the right and down for positive values, and never rolls")
{
    FlyCameraController controller;
    TransformComponent transform;

    controller.Update(Timestep(0.0f), {}, transform);
    CHECK(IsNear(Forward(transform), {0.0f, 0.0f, -1.0f}));

    controller.Update(Timestep(0.0f), {.Look = {900.0f, 0.0f}}, transform);
    CHECK(controller.GetYaw() == -90.0f);
    CHECK(IsNear(Forward(transform), {1.0f, 0.0f, 0.0f}));

    controller.Update(Timestep(0.0f), {.Look = {0.0f, 300.0f}}, transform);
    CHECK(IsNear(controller.GetPitch(), -30.0f));
    CHECK(Forward(transform).y < -0.49f);
    CHECK(IsNear(Right(transform), {0.0f, 0.0f, 1.0f}));
}

TEST_CASE("The pitch of a fly camera stops at the most it can look up or down")
{
    FlyCameraController controller({.MaxPitch = 60.0f});
    TransformComponent transform;

    controller.Update(Timestep(0.0f), {.Look = {0.0f, -10000.0f}}, transform);
    CHECK(controller.GetPitch() == 60.0f);
    controller.SetRotation(10.0f, -100.0f);
    CHECK(controller.GetYaw() == 10.0f);
    CHECK(controller.GetPitch() == -60.0f);

    controller.SetSettings({.MaxPitch = 120.0f});
    controller.SetRotation(0.0f, 100.0f);
    CHECK(controller.GetPitch() == 90.0f);
    controller.SetSettings({.MaxPitch = -30.0f});
    controller.SetRotation(0.0f, 100.0f);
    CHECK(controller.GetPitch() == 30.0f);
    CHECK(controller.GetSettings().MaxPitch == -30.0f);
}

TEST_CASE("A fly camera moves along its view, except up and down along the world Y axis, and keeps its scale")
{
    FlyCameraController controller;
    controller.SetRotation(-90.0f, -45.0f);
    TransformComponent transform{.Scale = glm::vec3(2.0f)};

    controller.Update(Timestep(1.0f), {.Move = {0.0f, 0.0f, 1.0f}}, transform);
    CHECK(IsNear(transform.Position, glm::vec3(1.0f, -1.0f, 0.0f) * (5.0f / glm::sqrt(2.0f))));

    transform.Position = glm::vec3(0.0f);
    controller.Update(Timestep(0.5f), {.Move = {0.0f, 1.0f, 0.0f}}, transform);
    CHECK(IsNear(transform.Position, {0.0f, 2.5f, 0.0f}));

    transform.Position = glm::vec3(0.0f);
    controller.Update(Timestep(1.0f), {.Move = {-1.0f, 0.0f, 0.0f}}, transform);
    CHECK(IsNear(transform.Position, {0.0f, 0.0f, -5.0f}));
    CHECK(transform.Scale == glm::vec3(2.0f));
}

TEST_CASE("A fly camera moves at the same speed in a diagonal, slower with a short move and faster when asked")
{
    FlyCameraController controller({.Speed = 2.0f, .FastSpeed = 8.0f});
    TransformComponent transform;

    controller.Update(Timestep(1.0f), {.Move = {1.0f, 0.0f, 1.0f}}, transform);
    CHECK(IsNear(glm::length(transform.Position), 2.0f));

    transform.Position = glm::vec3(0.0f);
    controller.Update(Timestep(1.0f), {.Move = {0.0f, 0.0f, 0.25f}}, transform);
    CHECK(IsNear(transform.Position, {0.0f, 0.0f, -0.5f}));

    transform.Position = glm::vec3(0.0f);
    controller.Update(Timestep(1.0f), {.Move = {0.0f, 0.0f, 1.0f}, .Fast = true}, transform);
    CHECK(IsNear(transform.Position, {0.0f, 0.0f, -8.0f}));

    transform.Position = glm::vec3(0.0f);
    controller.Update(Timestep(1.0f), {}, transform);
    CHECK(transform.Position == glm::vec3(0.0f));
}

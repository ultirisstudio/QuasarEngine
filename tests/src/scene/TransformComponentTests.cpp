#include <quasar/scene/components/TransformComponent.h>

#include <doctest/doctest.h>

#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <cmath>

using Quasar::TransformComponent;

namespace
{
    bool IsNear(const glm::vec3& value, const glm::vec3& expected, float tolerance = 1e-5f)
    {
        return glm::distance(value, expected) < tolerance;
    }

    glm::vec3 Apply(const TransformComponent& transform, const glm::vec3& point)
    {
        return glm::vec3(transform.GetMatrix() * glm::vec4(point, 1.0f));
    }
}

TEST_CASE("A transform scales, then rotates, then moves a point")
{
    TransformComponent transform;
    transform.Position = {10.0f, 0.0f, 0.0f};
    transform.Rotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    transform.Scale = {2.0f, 3.0f, 4.0f};

    CHECK(IsNear(Apply(transform, {1.0f, 0.0f, 0.0f}), {10.0f, 2.0f, 0.0f}));
    CHECK(IsNear(Apply(transform, {0.0f, 1.0f, 0.0f}), {7.0f, 0.0f, 0.0f}));
    CHECK(IsNear(Apply(transform, {0.0f, 0.0f, 1.0f}), {10.0f, 0.0f, 4.0f}));
    CHECK(IsNear(Apply(TransformComponent{}, {1.0f, 2.0f, 3.0f}), {1.0f, 2.0f, 3.0f}));
}

TEST_CASE("The rotation of a transform can be set and read as angles in radians")
{
    TransformComponent transform;

    transform.SetEulerAngles({0.0f, glm::radians(90.0f), 0.0f});
    CHECK(IsNear(transform.Rotation * glm::vec3(0.0f, 0.0f, -1.0f), {-1.0f, 0.0f, 0.0f}));
    TransformComponent reread;
    reread.SetEulerAngles(transform.GetEulerAngles());
    CHECK(IsNear(reread.Rotation * glm::vec3(0.0f, 0.0f, -1.0f), {-1.0f, 0.0f, 0.0f}, 1e-3f));
    CHECK(IsNear(reread.Rotation * glm::vec3(0.0f, 1.0f, 0.0f), {0.0f, 1.0f, 0.0f}, 1e-3f));

    const glm::vec3 angles(glm::radians(20.0f), glm::radians(-35.0f), glm::radians(60.0f));
    transform.SetEulerAngles(angles);
    CHECK(IsNear(transform.GetEulerAngles(), angles));
    CHECK(IsNear(transform.Rotation * glm::vec3(1.0f, 0.0f, 0.0f), glm::quat(angles) * glm::vec3(1.0f, 0.0f, 0.0f)));
}

TEST_CASE("A transform is set back from its matrix, a mirror gives a negative X scale, and a zero scale keeps the rotation")
{
    const TransformComponent source{.Position = {1.0f, -2.0f, 3.0f},
                                    .Rotation = glm::angleAxis(glm::radians(40.0f), glm::normalize(glm::vec3(1.0f, 2.0f, 3.0f))),
                                    .Scale = {2.0f, 0.5f, 3.0f}};
    TransformComponent transform;
    transform.SetMatrix(source.GetMatrix());
    CHECK(IsNear(transform.Position, source.Position));
    CHECK(IsNear(transform.Scale, source.Scale));
    CHECK(std::abs(glm::dot(transform.Rotation, source.Rotation)) > 0.99999f);

    const TransformComponent mirrored{.Scale = {-2.0f, 1.0f, 1.0f}};
    transform.SetMatrix(mirrored.GetMatrix());
    CHECK(IsNear(transform.Scale, {-2.0f, 1.0f, 1.0f}));
    CHECK(std::abs(glm::dot(transform.Rotation, glm::quat(1.0f, 0.0f, 0.0f, 0.0f))) > 0.99999f);

    const glm::quat kept = glm::angleAxis(glm::radians(30.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    transform.Rotation = kept;
    transform.SetMatrix(TransformComponent{.Position = {4.0f, 5.0f, 6.0f}, .Scale = {1.0f, 0.0f, 1.0f}}.GetMatrix());
    CHECK(IsNear(transform.Position, {4.0f, 5.0f, 6.0f}));
    CHECK(IsNear(transform.Scale, {1.0f, 0.0f, 1.0f}));
    CHECK(transform.Rotation == kept);
}

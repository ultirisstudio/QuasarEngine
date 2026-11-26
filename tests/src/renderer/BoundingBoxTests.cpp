#include "helpers/MathHelpers.h"

#include <quasar/renderer/BoundingBox.h>

#include <doctest/doctest.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>

#include <array>
#include <cmath>

using Quasar::BoundingBox;
using TestHelpers::IsNear;

TEST_CASE("A bounding box holds all of its points, and is empty at the origin without points")
{
    constexpr std::array points{glm::vec3(1.0f, -2.0f, 3.0f), glm::vec3(-4.0f, 5.0f, 0.5f), glm::vec3(2.0f, 0.0f, -6.0f)};

    CHECK(BoundingBox::FromPoints(points) == BoundingBox{.Min = {-4.0f, -2.0f, -6.0f}, .Max = {2.0f, 5.0f, 3.0f}});
    CHECK(BoundingBox::FromPoints(std::span(points).first(1)) == BoundingBox{.Min = points[0], .Max = points[0]});
    CHECK(BoundingBox::FromPoints({}) == BoundingBox{});
}

TEST_CASE("A moved bounding box holds the moved corners of the box, as tightly as a box aligned with the axes can")
{
    const BoundingBox box{.Min = {-1.0f, -2.0f, -3.0f}, .Max = {1.0f, 2.0f, 3.0f}};

    const BoundingBox moved = box.Transformed(glm::translate(glm::mat4(1.0f), {10.0f, 0.0f, -5.0f}));
    CHECK(IsNear(moved.Min, {9.0f, -2.0f, -8.0f}));
    CHECK(IsNear(moved.Max, {11.0f, 2.0f, -2.0f}));

    const BoundingBox scaled = box.Transformed(glm::scale(glm::mat4(1.0f), {2.0f, -1.0f, 0.5f}));
    CHECK(IsNear(scaled.Min, {-2.0f, -2.0f, -1.5f}));
    CHECK(IsNear(scaled.Max, {2.0f, 2.0f, 1.5f}));

    const BoundingBox turned = box.Transformed(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), {0.0f, 0.0f, 1.0f}));
    CHECK(IsNear(turned.Min, {-2.0f, -1.0f, -3.0f}, 1e-4f));
    CHECK(IsNear(turned.Max, {2.0f, 1.0f, 3.0f}, 1e-4f));

    const BoundingBox diagonal =
        BoundingBox{.Min = glm::vec3(-1.0f), .Max = glm::vec3(1.0f)}.Transformed(glm::rotate(glm::mat4(1.0f), glm::radians(45.0f), {0.0f, 1.0f, 0.0f}));
    CHECK(IsNear(diagonal.Max, {std::sqrt(2.0f), 1.0f, std::sqrt(2.0f)}, 1e-4f));
    CHECK(IsNear(diagonal.Min, -diagonal.Max, 1e-4f));
}

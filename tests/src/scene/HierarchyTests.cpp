#include "helpers/LogHelpers.h"
#include "helpers/MathHelpers.h"

#include <quasar/log/MemoryLogSink.h>
#include <quasar/scene/Entity.h>
#include <quasar/scene/Scene.h>
#include <quasar/scene/components/RelationshipComponent.h>
#include <quasar/scene/components/TransformComponent.h>

#include <doctest/doctest.h>

#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <string>
#include <vector>

using Quasar::Entity;
using Quasar::RelationshipComponent;
using Quasar::Scene;
using Quasar::TransformComponent;
using TestHelpers::CountErrors;
using TestHelpers::IsNear;

namespace
{
    glm::vec3 WorldPosition(const Entity& entity)
    {
        return glm::vec3(entity.GetWorldMatrix()[3]);
    }
}

TEST_CASE("A new entity is at the root of its scene, without children")
{
    const TestHelpers::ScopedLog log;
    Scene scene;
    const Entity entity = scene.CreateEntity("Root");

    CHECK_FALSE(entity.GetParent().IsValid());
    CHECK(entity.GetChildren().empty());
    CHECK_FALSE(entity.Get<RelationshipComponent>().Parent.IsValid());
}

TEST_CASE("Giving a parent links both sides, keeps the order of the children and moves the entity from its old parent")
{
    const TestHelpers::ScopedLog log;
    Scene scene;
    const Entity first = scene.CreateEntity("First");
    const Entity second = scene.CreateEntity("Second");
    Entity a = scene.CreateEntity("A");
    Entity b = scene.CreateEntity("B");

    CHECK(b.SetParent(first));
    CHECK(a.SetParent(first));
    CHECK(a.GetParent() == first);
    CHECK(first.GetChildren() == std::vector<Entity>{b, a});
    CHECK(first.Get<RelationshipComponent>().Children == std::vector<Quasar::UUID>{b.GetUUID(), a.GetUUID()});
    CHECK(a.Get<RelationshipComponent>().Parent == first.GetUUID());

    CHECK(b.SetParent(first));
    CHECK(first.GetChildren() == std::vector<Entity>{b, a});

    CHECK(a.SetParent(second));
    CHECK(first.GetChildren() == std::vector<Entity>{b});
    CHECK(second.GetChildren() == std::vector<Entity>{a});

    CHECK(a.SetParent({}));
    CHECK_FALSE(a.GetParent().IsValid());
    CHECK(second.GetChildren().empty());
    CHECK(a.SetParent({}));
    CHECK(CountErrors() == 0);
}

TEST_CASE("An entity cannot be the parent of itself or of an ancestor, nor come from another scene")
{
    const TestHelpers::ScopedLog log;
    Scene scene;
    Scene other;
    Entity root = scene.CreateEntity("Root");
    Entity child = scene.CreateEntity("Child");
    Entity grandchild = scene.CreateEntity("Grandchild");
    REQUIRE(child.SetParent(root));
    REQUIRE(grandchild.SetParent(child));

    CHECK_FALSE(root.SetParent(root));
    CHECK_FALSE(root.SetParent(grandchild));
    CHECK_FALSE(child.SetParent(other.CreateEntity("Elsewhere")));
    CHECK_FALSE(Entity().SetParent(root));
    CHECK(CountErrors() == 4);

    CHECK_FALSE(root.GetParent().IsValid());
    CHECK(child.GetParent() == root);
    CHECK(root.GetChildren() == std::vector<Entity>{child});
    CHECK(grandchild.IsDescendantOf(root));
    CHECK(grandchild.IsDescendantOf(child));
    CHECK_FALSE(root.IsDescendantOf(grandchild));
    CHECK_FALSE(root.IsDescendantOf(root));
    CHECK_FALSE(grandchild.IsDescendantOf(Entity()));
}

TEST_CASE("The world matrix of an entity is the one of its transform after the ones of its ancestors")
{
    const TestHelpers::ScopedLog log;
    Scene scene;
    const Entity root = scene.CreateEntity("Root");
    Entity child = scene.CreateEntity("Child");
    Entity grandchild = scene.CreateEntity("Grandchild");
    root.Get<TransformComponent>() = {.Position = {10.0f, 0.0f, 0.0f}, .Rotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f))};
    child.Get<TransformComponent>() = {.Position = {0.0f, 0.0f, -2.0f}, .Scale = glm::vec3(2.0f)};
    grandchild.Get<TransformComponent>().Position = {1.0f, 0.0f, 0.0f};
    REQUIRE(child.SetParent(root, false));
    REQUIRE(grandchild.SetParent(child, false));

    CHECK(IsNear(WorldPosition(root), {10.0f, 0.0f, 0.0f}));
    CHECK(IsNear(WorldPosition(child), {8.0f, 0.0f, 0.0f}));
    CHECK(IsNear(WorldPosition(grandchild), {8.0f, 0.0f, -2.0f}));
    CHECK(IsNear(grandchild.GetWorldMatrix(),
                 root.GetWorldMatrix() * child.Get<TransformComponent>().GetMatrix() * grandchild.Get<TransformComponent>().GetMatrix()));

    const TransformComponent world = grandchild.GetWorldTransform();
    CHECK(IsNear(world.Position, {8.0f, 0.0f, -2.0f}));
    CHECK(IsNear(world.Scale, glm::vec3(2.0f)));
    CHECK(IsNear(world.Rotation * glm::vec3(1.0f, 0.0f, 0.0f), {0.0f, 0.0f, -1.0f}));

    root.Get<TransformComponent>().Position.y = 5.0f;
    CHECK(IsNear(WorldPosition(grandchild), {8.0f, 5.0f, -2.0f}));

    grandchild.Remove<TransformComponent>();
    CHECK(IsNear(grandchild.GetWorldMatrix(), child.GetWorldMatrix()));
}

TEST_CASE("A new parent keeps the entity at its place in the world, unless its transform is kept instead")
{
    const TestHelpers::ScopedLog log;
    Scene scene;
    const Entity parent = scene.CreateEntity("Parent");
    parent.Get<TransformComponent>() = {
        .Position = {5.0f, 0.0f, 0.0f}, .Rotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)), .Scale = glm::vec3(2.0f)};
    Entity kept = scene.CreateEntity("Kept");
    Entity moved = scene.CreateEntity("Moved");
    const TransformComponent start{.Position = {1.0f, 2.0f, 3.0f}, .Scale = glm::vec3(4.0f)};
    kept.Get<TransformComponent>() = start;
    moved.Get<TransformComponent>() = start;

    REQUIRE(kept.SetParent(parent));
    REQUIRE(moved.SetParent(parent, false));
    CHECK(IsNear(kept.GetWorldMatrix(), start.GetMatrix()));
    CHECK(IsNear(kept.Get<TransformComponent>().Scale, glm::vec3(2.0f)));
    CHECK(IsNear(moved.Get<TransformComponent>().Position, start.Position));
    CHECK(IsNear(moved.GetWorldMatrix(), parent.GetWorldMatrix() * start.GetMatrix()));

    const glm::mat4 world = moved.GetWorldMatrix();
    REQUIRE(moved.SetParent({}));
    CHECK(IsNear(moved.GetWorldMatrix(), world));
    CHECK(IsNear(moved.Get<TransformComponent>().Position, glm::vec3(world[3])));
}

TEST_CASE("Destroying an entity destroys its descendants and removes it from the children of its parent")
{
    const TestHelpers::ScopedLog log;
    Scene scene;
    const Entity root = scene.CreateEntity("Root");
    Entity child = scene.CreateEntity("Child");
    Entity sibling = scene.CreateEntity("Sibling");
    Entity grandchild = scene.CreateEntity("Grandchild");
    REQUIRE(child.SetParent(root));
    REQUIRE(sibling.SetParent(root));
    REQUIRE(grandchild.SetParent(child));
    const Quasar::UUID grandchildId = grandchild.GetUUID();

    scene.DestroyEntity(child);

    CHECK_FALSE(child.IsValid());
    CHECK_FALSE(grandchild.IsValid());
    CHECK_FALSE(scene.FindEntity(grandchildId).IsValid());
    CHECK(root.GetChildren() == std::vector<Entity>{sibling});
    CHECK(scene.GetEntityCount() == 2);
    CHECK(CountErrors() == 0);
}

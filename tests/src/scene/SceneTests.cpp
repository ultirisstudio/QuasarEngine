#include "helpers/LogHelpers.h"

#include <quasar/core/UUID.h>
#include <quasar/log/MemoryLogSink.h>
#include <quasar/scene/Entity.h>
#include <quasar/scene/Scene.h>
#include <quasar/scene/components/CameraComponent.h>
#include <quasar/scene/components/TransformComponent.h>

#include <doctest/doctest.h>

#include <glm/vec3.hpp>

#include <set>
#include <string>
#include <vector>

using Quasar::Entity;
using Quasar::Scene;
using Quasar::TransformComponent;
using Quasar::UUID;

namespace
{
    struct Health
    {
        int Value = 100;
    };

    struct Velocity
    {
        glm::vec3 Value{0.0f};
    };
}

TEST_CASE("A new entity has a unique UUID, its name and a transform at the origin")
{
    Scene scene;

    const Entity first = scene.CreateEntity("Player");
    const Entity second = scene.CreateEntity();

    REQUIRE(first.IsValid());
    REQUIRE(second.IsValid());
    CHECK(first.GetUUID().IsValid());
    CHECK(first.GetUUID() != second.GetUUID());
    CHECK(first.GetName() == "Player");
    CHECK(second.GetName() == "Entity");
    REQUIRE(first.Has<TransformComponent>());
    CHECK(first.Get<TransformComponent>().GetMatrix() == glm::mat4(1.0f));
    CHECK(scene.GetEntityCount() == 2);
    CHECK(first != second);
    CHECK(first.GetScene() == &scene);
}

TEST_CASE("An entity is found by its UUID until it is destroyed")
{
    Scene scene;
    const UUID id(42);
    const Entity entity = scene.CreateEntity(id, "Saved");
    REQUIRE(entity.IsValid());

    CHECK(scene.FindEntity(id) == entity);
    CHECK(scene.FindEntity(entity.GetUUID()).GetName() == "Saved");
    CHECK_FALSE(scene.FindEntity(UUID(7)).IsValid());

    scene.DestroyEntity(entity);

    CHECK_FALSE(entity.IsValid());
    CHECK_FALSE(static_cast<bool>(entity));
    CHECK_FALSE(scene.FindEntity(id).IsValid());
    CHECK(scene.GetEntityCount() == 0);
}

TEST_CASE("An entity is refused with an invalid UUID or a UUID already used")
{
    Scene scene;
    REQUIRE(scene.CreateEntity(UUID(5), "First").IsValid());

    for (const UUID id : {UUID(), UUID(5)})
    {
        const TestHelpers::ScopedLog log;
        CHECK_FALSE(scene.CreateEntity(id, "Second").IsValid());
        CHECK(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
    }
    CHECK(scene.GetEntityCount() == 1);
    CHECK(scene.FindEntity(UUID(5)).GetName() == "First");
}

TEST_CASE("Destroying an entity that is not in the scene is refused")
{
    Scene scene;
    Scene other;
    const Entity entity = scene.CreateEntity();
    const Entity elsewhere = other.CreateEntity();

    for (const Entity refused : {Entity(), elsewhere})
    {
        const TestHelpers::ScopedLog log;
        scene.DestroyEntity(refused);
        CHECK(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
    }
    scene.DestroyEntity(entity);
    {
        const TestHelpers::ScopedLog log;
        scene.DestroyEntity(entity);
        CHECK(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
    }
    CHECK(elsewhere.IsValid());
    CHECK(other.GetEntityCount() == 1);
}

TEST_CASE("Components are added, read, changed and removed")
{
    Scene scene;
    Entity entity = scene.CreateEntity();

    CHECK_FALSE(entity.Has<Health>());
    CHECK(entity.TryGet<Health>() == nullptr);

    entity.Add<Health>().Value = 50;
    entity.Get<Health>().Value -= 10;

    CHECK(entity.Has<Health>());
    REQUIRE(entity.TryGet<Health>() != nullptr);
    CHECK(entity.TryGet<Health>()->Value == 40);
    CHECK(entity.Remove<Health>());
    CHECK_FALSE(entity.Remove<Health>());
    CHECK_FALSE(entity.Has<Health>());
}

TEST_CASE("Adding a component that the entity already has keeps it and logs an error")
{
    const TestHelpers::ScopedLog log;
    Scene scene;
    Entity entity = scene.CreateEntity("Hero");
    entity.Add<Health>(Health{80});

    const Health& health = entity.Add<Health>(Health{10});

    CHECK(health.Value == 80);
    CHECK(entity.Get<Health>().Value == 80);
    CHECK(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
}

TEST_CASE("An invalid entity has no component")
{
    Scene scene;
    const Entity destroyed = scene.CreateEntity();
    scene.DestroyEntity(destroyed);

    for (const Entity entity : {Entity(), destroyed})
    {
        CHECK_FALSE(entity.Has<TransformComponent>());
        CHECK(entity.TryGet<TransformComponent>() == nullptr);
    }
}

TEST_CASE("ForEach visits the entities that have all the components, which it can change")
{
    Scene scene;
    Entity moving = scene.CreateEntity("Moving");
    moving.Add<Velocity>(Velocity{{1.0f, 0.0f, 0.0f}});
    Entity hurt = scene.CreateEntity("Hurt");
    hurt.Add<Velocity>(Velocity{{0.0f, 2.0f, 0.0f}});
    hurt.Add<Health>();
    scene.CreateEntity("Still");

    std::set<std::string> visited;
    scene.ForEach<TransformComponent, Velocity>(
        [&visited](Entity entity, TransformComponent& transform, const Velocity& velocity)
        {
            visited.insert(entity.GetName());
            transform.Position += velocity.Value;
        });

    CHECK(visited == std::set<std::string>{"Moving", "Hurt"});
    CHECK(moving.Get<TransformComponent>().Position == glm::vec3(1.0f, 0.0f, 0.0f));
    CHECK(hurt.Get<TransformComponent>().Position == glm::vec3(0.0f, 2.0f, 0.0f));

    int withHealth = 0;
    scene.ForEach<Health>([&withHealth](Entity /*entity*/, Health& /*health*/) { ++withHealth; });
    CHECK(withHealth == 1);
}

TEST_CASE("ForEach can destroy the entity it visits")
{
    Scene scene;
    std::vector<UUID> kept;
    for (int index = 0; index < 6; ++index)
    {
        Entity entity = scene.CreateEntity(std::to_string(index));
        entity.Add<Health>(Health{index % 2 == 0 ? 0 : 10});
        if (index % 2 != 0)
        {
            kept.push_back(entity.GetUUID());
        }
    }

    scene.ForEach<Health>(
        [&scene](Entity entity, const Health& health)
        {
            if (health.Value == 0)
            {
                scene.DestroyEntity(entity);
            }
        });

    CHECK(scene.GetEntityCount() == kept.size());
    for (const UUID id : kept)
    {
        CHECK(scene.FindEntity(id).IsValid());
    }
}

TEST_CASE("The primary camera is an entity whose camera component is primary")
{
    Scene scene;
    CHECK_FALSE(scene.FindPrimaryCamera().IsValid());

    Entity secondary = scene.CreateEntity("Secondary");
    secondary.Add<Quasar::CameraComponent>().Primary = false;
    scene.CreateEntity("Without camera");
    CHECK_FALSE(scene.FindPrimaryCamera().IsValid());

    Entity primary = scene.CreateEntity("Primary");
    primary.Add<Quasar::CameraComponent>();
    CHECK(scene.FindPrimaryCamera() == primary);

    primary.Get<Quasar::CameraComponent>().Primary = false;
    secondary.Get<Quasar::CameraComponent>().Primary = true;
    CHECK(scene.FindPrimaryCamera() == secondary);
}

TEST_CASE("The viewport size of a scene goes to the camera of every camera component")
{
    const TestHelpers::ScopedLog log;
    Scene scene;
    Entity first = scene.CreateEntity("First");
    Entity second = scene.CreateEntity("Second");
    first.Add<Quasar::CameraComponent>();
    second.Add<Quasar::CameraComponent>().Primary = false;
    scene.CreateEntity("Without camera");

    scene.SetViewportSize(400, 200);

    CHECK(first.Get<Quasar::CameraComponent>().Camera.GetAspectRatio() == 2.0f);
    CHECK(second.Get<Quasar::CameraComponent>().Camera.GetAspectRatio() == 2.0f);
}

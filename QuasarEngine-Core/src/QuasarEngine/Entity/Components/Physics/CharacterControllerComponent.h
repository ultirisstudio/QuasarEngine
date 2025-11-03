#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <PxPhysicsAPI.h>

#include <characterkinematic/PxController.h>
#include <characterkinematic/PxCapsuleController.h>
#include <characterkinematic/PxControllerManager.h>
#include <QuasarEngine/Physic/PhysXQueryUtils.h>

namespace QuasarEngine
{
    class CharacterControllerComponent {
    public:
        CharacterControllerComponent() = default;
        ~CharacterControllerComponent() = default;

        bool Create(physx::PxPhysics* physics, physx::PxControllerManager* mgr, physx::PxMaterial* material,
            const glm::vec3& startPos);

        void Destroy();

        bool Move(const glm::vec3& displacement, float dt, float minDist = 0.001f);
        void SetPosition(const glm::vec3& pos);
        glm::vec3 GetPosition() const;

        void ResizeHeight(float height);
        void SetRadius(float radius);

        void SetStepOffset(float step) { m_StepOffset = step; ApplyRuntimeParams(); }
        void SetSlopeLimitDeg(float deg) { m_SlopeLimitDeg = deg; ApplyRuntimeParams(); }
        void SetContactOffset(float off) { m_ContactOffset = off; ApplyRuntimeParams(); }

        void SetLayerMask(uint32_t layer, uint32_t mask) { m_Layer = layer; m_Mask = mask; }
        void SetIncludeTriggers(bool v) { m_IncludeTriggers = v; }

        bool IsGrounded() const { return m_Grounded; }
        physx::PxCapsuleController* GetPx() const { return m_Controller; }

    private:
        void ApplyRuntimeParams();
        float SlopeLimitCos() const;

    private:
        physx::PxControllerManager* m_Manager = nullptr;
        physx::PxCapsuleController* m_Controller = nullptr;

        float m_Radius = 0.4f;
        float m_Height = 1.8f;
        float m_StepOffset = 0.3f;
        float m_SlopeLimitDeg = 45.0f;
        float m_ContactOffset = 0.1f;

        uint32_t m_Layer = 1;
        uint32_t m_Mask = 0xFFFFFFFF;
        bool m_IncludeTriggers = false;

        bool m_Grounded = false;
    };
}
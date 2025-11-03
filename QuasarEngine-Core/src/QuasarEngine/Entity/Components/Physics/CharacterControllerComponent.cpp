#include "qepch.h"
#include "CharacterControllerComponent.h"
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <characterkinematic/PxController.h>
#include <characterkinematic/PxCapsuleController.h>
#include <characterkinematic/PxControllerManager.h>

namespace QuasarEngine
{
    static physx::PxCapsuleClimbingMode::Enum kClimbMode = physx::PxCapsuleClimbingMode::eCONSTRAINED;

    float CharacterControllerComponent::SlopeLimitCos() const {
        const float rad = glm::radians(m_SlopeLimitDeg);
        return cosf(rad);
    }

    bool CharacterControllerComponent::Create(physx::PxPhysics* physics,
        physx::PxControllerManager* mgr,
        physx::PxMaterial* material,
        const glm::vec3& startPos)
    {
        if (!physics || !mgr || !material) return false;
        Destroy();

        physx::PxCapsuleControllerDesc desc;
        desc.height = m_Height;
        desc.radius = m_Radius;
        desc.material = material;
        desc.contactOffset = m_ContactOffset;
        desc.stepOffset = m_StepOffset;
        desc.slopeLimit = SlopeLimitCos();
        desc.upDirection = physx::PxVec3(0, 1, 0);
        desc.position = physx::PxExtendedVec3(startPos.x, startPos.y, startPos.z);
        desc.climbingMode = physx::PxCapsuleClimbingMode::eCONSTRAINED;
        desc.scaleCoeff = 0.9f;

        if (!desc.isValid()) return false;

        physx::PxController* base = mgr->createController(desc);
        if (!base) return false;

        if (base->getType() != physx::PxControllerShapeType::eCAPSULE) {
            base->release();
            return false;
        }

        m_Controller = static_cast<physx::PxCapsuleController*>(base);
        m_Manager = mgr;
        m_Grounded = false;
        return true;
    }

    void CharacterControllerComponent::Destroy() {
        if (m_Controller) {
            m_Controller->release();
            m_Controller = nullptr;
        }
        m_Manager = nullptr;
        m_Grounded = false;
    }

    void CharacterControllerComponent::ApplyRuntimeParams() {
        if (!m_Controller) return;
        m_Controller->setStepOffset(m_StepOffset);
        m_Controller->setContactOffset(m_ContactOffset);
        m_Controller->setSlopeLimit(SlopeLimitCos());
    }

    bool CharacterControllerComponent::Move(const glm::vec3& displacement, float dt, float minDist)
    {
        if (!m_Controller) return false;

        QueryFilterCB qcb;
        qcb.includeTriggers = m_IncludeTriggers;
        qcb.ignoreActor = nullptr;

        physx::PxFilterData qfd;
        qfd.word0 = m_Layer;
        qfd.word1 = m_Mask;
        qfd.word2 = 0;
        qfd.word3 = 0;

        physx::PxControllerFilters filters;
        filters.mFilterData = &qfd;
        filters.mFilterCallback = &qcb;
        filters.mCCTFilterCallback = nullptr;

        const physx::PxVec3 disp(displacement.x, displacement.y, displacement.z);
        const physx::PxControllerCollisionFlags flags = m_Controller->move(disp, minDist, dt, filters);

        m_Grounded = (flags & physx::PxControllerCollisionFlag::eCOLLISION_DOWN);
        return flags.isSet(physx::PxControllerCollisionFlag::eCOLLISION_SIDES)
            || flags.isSet(physx::PxControllerCollisionFlag::eCOLLISION_UP)
            || flags.isSet(physx::PxControllerCollisionFlag::eCOLLISION_DOWN);
    }

    void CharacterControllerComponent::SetPosition(const glm::vec3& pos)
    {
        if (!m_Controller) return;
        m_Controller->setPosition(physx::PxExtendedVec3(pos.x, pos.y, pos.z));
    }

    glm::vec3 CharacterControllerComponent::GetPosition() const
    {
        if (!m_Controller) return glm::vec3(0);
        const physx::PxExtendedVec3 p = m_Controller->getPosition();
        return glm::vec3((float)p.x, (float)p.y, (float)p.z);
    }

    void CharacterControllerComponent::ResizeHeight(float height)
    {
        if (!m_Controller) { m_Height = height; return; }
        m_Height = height;
        m_Controller->setHeight(m_Height);
    }

    void CharacterControllerComponent::SetRadius(float radius)
    {
        if (!m_Controller) { m_Radius = radius; return; }
        m_Radius = radius;
        m_Controller->setRadius(m_Radius);
    }
}
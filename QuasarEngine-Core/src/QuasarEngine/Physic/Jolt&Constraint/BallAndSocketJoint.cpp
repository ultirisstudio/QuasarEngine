#include "qepch.h"
#include "BallAndSocketJoint.h"
#include "QuasarEngine/Physic/RigidBody.h"
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>

namespace QuasarEngine {

    BallAndSocketJoint::BallAndSocketJoint(RigidBody* A, RigidBody* B, const glm::vec3& anchor)
        : Joint(A, B)
    {
        glm::mat4 transformA = glm::translate(A->position) * glm::toMat4(A->orientation);
        glm::mat4 transformB = glm::translate(B->position) * glm::toMat4(B->orientation);

        // On convertit l’ancre globale en coordonnées locales
        mLocalAnchorA = glm::inverse(transformA) * glm::vec4(anchor, 1.0f);
        mLocalAnchorB = glm::inverse(transformB) * glm::vec4(anchor, 1.0f);
    }

    // ---------------------------------------------------------------------------------------
    // PreSolve : calcul des erreurs de position à corriger via des impulsions
    // ---------------------------------------------------------------------------------------
    void BallAndSocketJoint::PreSolve(float dt) {
        if (!mBodyA || !mBodyB) return;

        glm::mat4 transformA = glm::translate(mBodyA->position) * glm::toMat4(mBodyA->orientation);
        glm::mat4 transformB = glm::translate(mBodyB->position) * glm::toMat4(mBodyB->orientation);

        mWorldAnchorA = glm::vec3(transformA * glm::vec4(mLocalAnchorA, 1.0f));
        mWorldAnchorB = glm::vec3(transformB * glm::vec4(mLocalAnchorB, 1.0f));

        // Erreur de position (vecteur à corriger)
        glm::vec3 error = mWorldAnchorB - mWorldAnchorA;

        // Calcul du biais (terme correctif) basé sur l’erreur
        mBiasTranslation = -(mBeta / dt) * error;

        // Reset de l’impulsion accumulée
        mAccumulatedImpulseTranslation = glm::vec3(0.0f);
    }

    // ---------------------------------------------------------------------------------------
    // SolveConstraint : appliquer une impulsion pour rapprocher les deux points d’ancrage
    // ---------------------------------------------------------------------------------------
    void BallAndSocketJoint::SolveConstraint() {
        if (!mBodyA || !mBodyB) return;

        glm::vec3 ra = mWorldAnchorA - mBodyA->position;
        glm::vec3 rb = mWorldAnchorB - mBodyB->position;

        // Vitesse relative au point de contact
        glm::vec3 va = mBodyA->linearVelocity + glm::cross(mBodyA->angularVelocity, ra);
        glm::vec3 vb = mBodyB->linearVelocity + glm::cross(mBodyB->angularVelocity, rb);
        glm::vec3 relativeVelocity = vb - va;

        // Calcul de l’impulsion requise
        glm::vec3 impulse = -(relativeVelocity + mBiasTranslation);

        float totalInvMass = mBodyA->inverseMass + mBodyB->inverseMass;

        if (totalInvMass > 0.0f) {
            impulse /= totalInvMass;

            // Appliquer l’impulsion linéaire
            mBodyA->linearVelocity -= impulse * mBodyA->inverseMass;
            mBodyB->linearVelocity += impulse * mBodyB->inverseMass;

            // Appliquer l’impulsion angulaire
            mBodyA->angularVelocity -= mBodyA->inverseInertiaTensorWorld * glm::cross(ra, impulse);
            mBodyB->angularVelocity += mBodyB->inverseInertiaTensorWorld * glm::cross(rb, impulse);
        }

        // Stockage pour le warmstarting (optionnel ici)
        mAccumulatedImpulseTranslation += impulse;
    }

}
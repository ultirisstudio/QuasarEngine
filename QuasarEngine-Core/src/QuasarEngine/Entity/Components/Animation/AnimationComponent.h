#pragma once

#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <glm/glm.hpp>

#include <QuasarEngine/Entity/Component.h>
#include <QuasarEngine/Resources/Model.h>
#include <QuasarEngine/Animation/Animation.h>

namespace QuasarEngine {

    class AnimationComponent : public Component
    {
    public:
        AnimationComponent() = default;
        explicit AnimationComponent(std::string modelAssetId);

        void SetModelAssetId(std::string id);
        const std::string& GetModelAssetId() const { return m_ModelAssetId; }

        void SetClips(std::vector<AnimationClip> clips);
        const std::vector<AnimationClip>& GetClips() const { return m_Clips; }

        void AppendClips(std::vector<AnimationClip> clips, bool dedupeByName = true, const std::string& namePrefix = "");

        void AppendClipsFromAsset(std::string animationAssetId, bool dedupeByName = true, const std::string& namePrefix = "");

        void Play(size_t clipIndex, bool loop = true, float speed = 1.0f);
        void Stop();
        void Pause() { m_Playing = false; }

        bool IsPlaying() const { return m_Playing; }
        bool IsPaused()  const { return !m_Playing && m_CurrentClip != (size_t)-1; }
        size_t CurrentClipIndex() const { return m_CurrentClip; }

        void Update(double dtSeconds);

        bool GetLoop()  const { return m_Loop; }
        void SetLoop(bool b) { m_Loop = b; }

        float GetSpeed() const { return m_Speed; }
        void SetSpeed(float s) { m_Speed = s; }

        double GetTimeSeconds() const { return m_TimeSec; }
        void SetTimeSeconds(double t) { m_TimeSec = std::max(0.0, t); }

        void Resume();

        size_t GetClipCount() const { return m_Clips.size(); }

        const AnimationClip* GetClip(size_t i) const
        {
            return (i < m_Clips.size()) ? &m_Clips[i] : nullptr;
        }

        const AnimationClip* GetCurrentClip() const
        {
            return (m_CurrentClip < m_Clips.size()) ? &m_Clips[m_CurrentClip] : nullptr;
        }

        const std::vector<glm::mat4>& GetFinalBoneMatrices() const { return m_FinalBoneMatrices; }

        void SetInPlace(bool v) { m_InPlace = v; }
        bool GetInPlace() const { return m_InPlace; }
        bool IsInPlace()  const { return m_InPlace; }

        void SetRootBoneName(std::string name) { m_RootMotionBone = std::move(name); }
        const std::string& GetRootBoneName() const { return m_RootMotionBone; }

    private:
        void EnsureModel();
        void EnsureBuffers();

        void ComputePoseRecursive(const ModelNode* node,
            const glm::mat4& parent,
            float animTimeTicks,
            bool inPlace,
            const std::string& rootBoneName);

        std::string ResolveRootMotionNode(const AnimationClip& clip) const;

        static void DecomposeTRS(const glm::mat4& m, glm::vec3& T, glm::quat& R, glm::vec3& S);
        static glm::mat4 ComposeTRS(const glm::vec3& T, const glm::quat& R, const glm::vec3& S);

    private:
        std::string m_ModelAssetId;
        std::weak_ptr<Model> m_Model;

        std::vector<AnimationClip> m_Clips;
        size_t  m_CurrentClip = (size_t)-1;
        bool    m_Loop = true;
        bool    m_Playing = false;
        float   m_Speed = 1.0f;
        double  m_TimeSec = 0.0;

        bool m_InPlace = false;
        std::string m_RootMotionBone;

        std::vector<glm::mat4> m_FinalBoneMatrices;
    };
}
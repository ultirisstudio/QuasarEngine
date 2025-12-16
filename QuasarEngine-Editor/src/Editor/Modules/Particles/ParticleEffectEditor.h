#pragma once

#include <memory>

#include <Editor/Modules/IEditorModule.h>

#include <imgui/imgui.h>

namespace QuasarEngine
{
    class ParticleEffect;

    class ParticleEffectEditor : public IEditorModule
    {
    public:
        ParticleEffectEditor(EditorContext& context);
        ~ParticleEffectEditor() override;

        void SetCurrentEffect(const std::shared_ptr<ParticleEffect>& effect)
        {
            m_Effect = effect;
            m_SelectedEmitter = 0;
        }

        std::shared_ptr<ParticleEffect> GetCurrentEffect() const { return m_Effect; }

        void Update(double dt) override {}
        void Render() override {}
        void RenderUI() override;

    private:
        void RenderEmitterList();
        void RenderEmitterInspector();
        void RenderPreview();

    private:
        std::shared_ptr<ParticleEffect> m_Effect;
        int m_SelectedEmitter = -1;

        bool m_Playing = true;
        bool m_LoopPreview = true;
        float m_TimeScale = 1.0f;
        float m_PreviewTime = 0.0f;
    };
}

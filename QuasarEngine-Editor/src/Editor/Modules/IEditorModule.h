#pragma once

#include <string>
#include <memory>

#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Events/Event.h>
#include <QuasarEngine/Scene/SceneManager.h>
#include <QuasarEngine/Scene/Importer/AssetImporter.h>

#include <Editor/EditorCamera.h>

namespace QuasarEngine
{
    struct EditorContext
    {
        std::unique_ptr<SceneManager> sceneManager;
        std::unique_ptr<EditorCamera> editorCamera;
        std::unique_ptr<AssetImporter> assetImporter;

        Entity selectedEntity;

        std::string projectPath;

        EditorContext() = default;
    };

    class IEditorModule {
    public:
		IEditorModule(EditorContext& context) : m_Context(context) {}
        virtual ~IEditorModule() = default;

        virtual void Update(double dt) {}
        virtual void Render() {}
        virtual void RenderUI() {}

		virtual void OnEvent(Event& e) {}

        bool& OpenFlag() { return m_Open; }
		bool& FocusedFlag() { return m_Focused; }
		bool& HoveredFlag() { return m_Hovered; }

    protected:
        bool m_Open = true;
		bool m_Focused = false;
		bool m_Hovered = false;

		EditorContext& m_Context;
    };
}
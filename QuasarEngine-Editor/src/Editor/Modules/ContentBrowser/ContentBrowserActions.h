#pragma once

#include <filesystem>
#include <string>
#include <vector>
#include <memory>
#include <system_error>

#include "ContentBrowserModel.h"

#include <Editor/Modules/TextureViewer/TextureViewer.h>
#include <Editor/Modules/CodeEditor/CodeEditor.h>
#include <Editor/Modules/Particles/ParticleEffectEditor.h>

#include <QuasarEngine/Scene/Importer/AssetImporter.h>
#include <QuasarEngine/Scene/Importer/TextureConfigImporter.h>

namespace QuasarEngine
{
    struct EditorContext;

    class ContentBrowserActions
    {
    public:
        ContentBrowserActions(EditorContext& ctx, ContentBrowserModel& model);

        void Update(double dt);
        void RenderSubEditors();

        void CopySelection();
        void CutSelection();
        void PasteInto(const std::filesystem::path& destDir);

        void Duplicate(const std::filesystem::path& src);
        void CommitRename(const std::filesystem::path& oldPath, const std::string& newName);
        void ConfirmDeleteToTrash();
        void CancelDelete();

        void RevealInExplorer(const std::filesystem::path& path);

        void MoveItemToDir(const std::filesystem::path& src, const std::filesystem::path& destDir);

        void CreateNewFolder();
        void CreateNewTextFile();
        void CreateNewLuaScript();
        void CreateNewShader();
        void CreateNewMaterial();
        void CreateNewScene();
        void CreateNewParticleEffect();

        void OpenOnDoubleClick(const ContentBrowserModel::Entry& e);

    private:
        static bool CopyRec(const std::filesystem::path& src, const std::filesystem::path& dst, std::error_code& ec);
        static bool MoveRec(const std::filesystem::path& src, const std::filesystem::path& dst, std::error_code& ec);
        static bool MoveToTrash(const std::filesystem::path& p, const std::filesystem::path& trash, std::string& err);

        void ProcessTextureLoadBudget(int budgetLoads, int budgetIconUpdates);

        EditorContext& m_Ctx;
        ContentBrowserModel& m_Model;

        std::shared_ptr<TextureViewer> m_TextureViewer;
        std::shared_ptr<CodeEditor> m_CodeEditor;
        std::shared_ptr<ParticleEffectEditor> m_ParticleEffectEditor;
    };
}
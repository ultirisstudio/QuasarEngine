#include "ContentBrowserActions.h"

#include <fstream>

#include <QuasarEngine/Resources/Particles/ParticleEffectSerializer.h>
#include <QuasarEngine/Resources/Particles/ParticleEffect.h>

#if defined(_WIN32)
#include <Windows.h>
#endif

namespace QuasarEngine
{
    namespace
    {
        std::shared_ptr<ParticleEffect> CreateDefaultParticleEffectForPath(const std::filesystem::path& filePath)
        {
            auto effect = std::make_shared<ParticleEffect>();
            effect->SetName(filePath.stem().string());

            auto& emitterAsset = effect->AddEmitter("DefaultEmitter");
            auto& s = emitterAsset.settings;

            s.enabled = true;
            s.loop = true;
            s.duration = 0.0f;

            s.simulationSpace = ParticleEmitterSimulationSpace::World;

            s.maxParticles = 512;
            s.spawnRate = 50.0f;

            s.shape = ParticleEmitterShape::Sphere;
            s.positionOffset = glm::vec3(0.0f);
            s.sphereRadius = 0.25f;

            s.baseDirection = glm::vec3(0.0f, 1.0f, 0.0f);
            s.spreadAngleDeg = 35.0f;
            s.speedMin = 1.0f;
            s.speedMax = 2.5f;
            s.velocityRandomness = 0.25f;

            s.lifeMin = 1.0f;
            s.lifeMax = 2.0f;

            s.startSizeMin = 0.25f;
            s.startSizeMax = 0.35f;
            s.endSizeMin = 0.6f;
            s.endSizeMax = 0.8f;

            s.sizeOverLifeExponent = 1.0f;
            s.alphaOverLifeExponent = 2.0f;

            s.randomRotation = true;
            s.angularVelocityMin = -1.0f;
            s.angularVelocityMax = 1.0f;

            s.colorStart = glm::vec4(0.9f, 0.9f, 0.9f, 0.9f);
            s.colorEnd = glm::vec4(0.3f, 0.3f, 0.3f, 0.0f);

            s.gravity = glm::vec3(0.0f);
            s.linearDrag = 0.25f;
            s.wind = glm::vec3(0.0f);

            s.turbulenceStrength = 0.5f;
            s.turbulenceFrequency = 1.0f;
            s.turbulenceScale = 1.0f;

            s.blendMode = ParticleEmitterBlendMode::Alpha;
            s.softFade = true;

            emitterAsset.textureId.clear();
            emitterAsset.texturePath.clear();

            return effect;
        }
    }

    ContentBrowserActions::ContentBrowserActions(EditorContext& ctx, ContentBrowserModel& model)
        : m_Ctx(ctx), m_Model(model)
    {
    }

    bool ContentBrowserActions::CopyRec(const std::filesystem::path& src, const std::filesystem::path& dst, std::error_code& ec)
    {
        if (std::filesystem::is_directory(src, ec)) {
            if (ec) return false;
            std::filesystem::create_directories(dst, ec);
            if (ec) return false;

            for (auto it = std::filesystem::directory_iterator(src, ec);
                !ec && it != std::filesystem::directory_iterator(); ++it)
            {
                if (!CopyRec(it->path(), dst / it->path().filename(), ec)) return false;
            }
            return !ec;
        }

        std::filesystem::create_directories(dst.parent_path(), ec);
        if (ec) return false;

        std::filesystem::copy_file(src, dst, std::filesystem::copy_options::overwrite_existing, ec);
        return !ec;
    }

    bool ContentBrowserActions::MoveRec(const std::filesystem::path& src, const std::filesystem::path& dst, std::error_code& ec)
    {
        std::filesystem::rename(src, dst, ec);
        if (!ec) return true;

        ec.clear();
        if (!CopyRec(src, dst, ec) || ec) return false;

        ec.clear();
        std::filesystem::remove_all(src, ec);
        return !ec;
    }

    bool ContentBrowserActions::MoveToTrash(const std::filesystem::path& p, const std::filesystem::path& trash, std::string& err)
    {
        std::error_code ec;
        auto target = ContentBrowserModel::UniqueNameInDir(trash, p.filename().string());

        std::filesystem::rename(p, target, ec);
        if (!ec) return true;

        ec.clear();
        if (!CopyRec(p, target, ec) || ec) { err = ec.message(); return false; }

        ec.clear();
        std::filesystem::remove_all(p, ec);
        if (ec) { err = ec.message(); return false; }

        return true;
    }

    void ContentBrowserActions::Update(double dt)
    {
        if (m_TextureViewer) m_TextureViewer->Update(dt);

        ProcessTextureLoadBudget(2, 4);
    }

    void ContentBrowserActions::ProcessTextureLoadBudget(int budgetLoads, int budgetIconUpdates)
    {
        while (budgetLoads-- > 0)
        {
            std::string texId;
            std::filesystem::path absPath;
            if (!m_Model.PopNextTextureToLoad(texId, absPath))
                break;

            if (texId.empty()) continue;
            if (AssetManager::Instance().isAssetLoaded(texId)) {
                m_Model.EnqueueTextureIconUpdate(texId);
                continue;
            }

            TextureSpecification spec = TextureConfigImporter::ImportTextureConfig(absPath.generic_string());
            AssetToLoad asset{};
            asset.id = texId;
            asset.path = absPath.generic_string();
            asset.type = AssetType::TEXTURE;
            asset.spec = spec;

            AssetManager::Instance().loadAsset(asset);
            m_Model.MarkTextureLoadRequested(texId);
            m_Model.EnqueueTextureIconUpdate(texId);
        }

        m_Model.UpdateTextureIconsBudget(budgetIconUpdates);
    }

    void ContentBrowserActions::RenderSubEditors()
    {
        if (m_TextureViewer) {
            if (!m_TextureViewer->OpenFlag()) m_TextureViewer.reset();
            else m_TextureViewer->RenderUI();
        }
        if (m_CodeEditor) m_CodeEditor->RenderUI();
        if (m_ParticleEffectEditor) m_ParticleEffectEditor->RenderUI();
    }

    void ContentBrowserActions::CopySelection()
    {
        auto& cb = m_Model.ClipboardState();
        cb.mode = ContentBrowserModel::ClipMode::Copy;
        cb.items.clear();

        for (const auto& key : m_Model.Selection())
            cb.items.push_back(std::filesystem::path(key));
    }

    void ContentBrowserActions::CutSelection()
    {
        auto& cb = m_Model.ClipboardState();
        cb.mode = ContentBrowserModel::ClipMode::Cut;
        cb.items.clear();

        for (const auto& key : m_Model.Selection())
            cb.items.push_back(std::filesystem::path(key));
    }

    void ContentBrowserActions::PasteInto(const std::filesystem::path& destDir)
    {
        auto& cb = m_Model.ClipboardState();
        if (cb.mode == ContentBrowserModel::ClipMode::None || cb.items.empty()) return;

        std::error_code ec;
        for (const auto& src : cb.items)
        {
            if (src.empty()) continue;

            auto desired = destDir / src.filename();
            auto dst = std::filesystem::exists(desired) ? ContentBrowserModel::UniqueNameInDir(destDir, src.filename().string())
                : desired;

            ec.clear();
            if (cb.mode == ContentBrowserModel::ClipMode::Copy) {
                CopyRec(src, dst, ec);
            }
            else {
                MoveRec(src, dst, ec);
            }

            if (ec) m_Model.SetLastError(ec.message());
        }

        if (cb.mode == ContentBrowserModel::ClipMode::Cut) cb = {};
        m_Model.MarkCacheDirty();
    }

    void ContentBrowserActions::Duplicate(const std::filesystem::path& src)
    {
        std::error_code ec;
        auto base = src.filename().string();
        auto dst = ContentBrowserModel::UniqueNameInDir(src.parent_path(), base + " - Copy");
        CopyRec(src, dst, ec);
        if (ec) m_Model.SetLastError(ec.message());
        m_Model.MarkCacheDirty();
    }

    void ContentBrowserActions::CommitRename(const std::filesystem::path& oldPath, const std::string& newName)
    {
        if (newName.empty()) return;

        const auto dst = oldPath.parent_path() / newName;
        if (dst == oldPath) {
            m_Model.CancelRename();
            return;
        }

        std::error_code ec;
        if (std::filesystem::exists(dst, ec) && !ec) {
            m_Model.SetLastError("Target already exists");
            return;
        }

        ec.clear();
        std::filesystem::rename(oldPath, dst, ec);
        if (ec) m_Model.SetLastError(ec.message());

        m_Model.CancelRename();
        m_Model.MarkCacheDirty();
    }

    void ContentBrowserActions::ConfirmDeleteToTrash()
    {
        const auto& pending = m_Model.PendingDelete();
        if (pending.empty()) return;

        for (const auto& p : pending)
        {
            std::string err;
            if (!MoveToTrash(p, m_Model.TrashDirectory(), err))
                m_Model.SetLastError(err);
        }

        m_Model.ClearPendingDelete();
        m_Model.ClearSelection();
        m_Model.MarkCacheDirty();
    }

    void ContentBrowserActions::CancelDelete()
    {
        m_Model.ClearPendingDelete();
    }

    void ContentBrowserActions::RevealInExplorer(const std::filesystem::path& path)
    {
#if defined(_WIN32)
        std::string cmd = "explorer /select,\"" + path.string() + "\"";
        system(cmd.c_str());
#elif defined(__APPLE__)
        std::string cmd = "open -R \"" + path.string() + "\"";
        system(cmd.c_str());
#else
        std::string cmd = "xdg-open \"" + path.parent_path().string() + "\"";
        system(cmd.c_str());
#endif
    }

    void ContentBrowserActions::MoveItemToDir(const std::filesystem::path& src, const std::filesystem::path& destDir)
    {
        if (src.empty()) return;
        if (destDir.empty()) return;
        if (src == destDir) return;

        std::error_code ec;
        auto srcAbs = std::filesystem::weakly_canonical(src, ec);
        auto dstAbs = std::filesystem::weakly_canonical(destDir, ec);

        bool intoSelf = false;
        if (!ec) {
            auto s = srcAbs.generic_string();
            auto d = dstAbs.generic_string();
            if (!s.empty() && !d.empty()) {
                if (d.rfind(s + "/", 0) == 0) intoSelf = true;
            }
        }
        if (intoSelf) return;

        auto desired = destDir / src.filename();
        auto dst = std::filesystem::exists(desired) ? ContentBrowserModel::UniqueNameInDir(destDir, src.filename().string())
            : desired;

        ec.clear();
        MoveRec(src, dst, ec);
        if (ec) m_Model.SetLastError(ec.message());
        m_Model.MarkCacheDirty();
    }

    void ContentBrowserActions::CreateNewFolder()
    {
        auto dir = m_Model.CurrentDirectory();
        std::filesystem::path newFolder = dir / "New Folder";
        int counter = 1;
        while (std::filesystem::exists(newFolder))
            newFolder = dir / ("New Folder " + std::to_string(counter++));

        std::error_code ec;
        std::filesystem::create_directory(newFolder, ec);
        if (ec) m_Model.SetLastError(ec.message());
        m_Model.MarkCacheDirty();
    }

    void ContentBrowserActions::CreateNewTextFile()
    {
        auto dir = m_Model.CurrentDirectory();
        std::filesystem::path nf = dir / "NewFile.txt";
        int c = 1;
        while (std::filesystem::exists(nf))
            nf = dir / ("NewFile" + std::to_string(c++) + ".txt");

        std::ofstream ofs(nf);
        ofs << "";
        m_Model.MarkCacheDirty();
    }

    void ContentBrowserActions::CreateNewLuaScript()
    {
        auto dir = m_Model.CurrentDirectory();
        std::filesystem::path nf = dir / "NewScript.lua";
        int c = 1;
        while (std::filesystem::exists(nf))
            nf = dir / ("NewScript" + std::to_string(c++) + ".lua");

        std::ofstream ofs(nf);
        ofs << "-- Default Lua Script\n"
            "function OnStart()\n"
            "    print(\"Hello from Lua!\")\n"
            "end\n\n"
            "function OnUpdate(dt)\n"
            "    -- Your update logic here\n"
            "end\n";
        m_Model.MarkCacheDirty();
    }

    void ContentBrowserActions::CreateNewShader()
    {
        auto dir = m_Model.CurrentDirectory();
        std::filesystem::path nf = dir / "NewShader.glsl";
        int c = 1;
        while (std::filesystem::exists(nf))
            nf = dir / ("NewShader" + std::to_string(c++) + ".glsl");

        std::ofstream ofs(nf);
        ofs << "// vertex/fragment\n";
        m_Model.MarkCacheDirty();
    }

    void ContentBrowserActions::CreateNewMaterial()
    {
        auto dir = m_Model.CurrentDirectory();
        std::filesystem::path nf = dir / "NewMaterial.mat";
        int c = 1;
        while (std::filesystem::exists(nf))
            nf = dir / ("NewMaterial" + std::to_string(c++) + ".mat");

        std::ofstream ofs(nf);
        ofs << "albedo=1,1,1\nmetallic=0\nroughness=1\n";
        m_Model.MarkCacheDirty();
    }

    void ContentBrowserActions::CreateNewScene()
    {
        auto dir = m_Model.CurrentDirectory();
        std::filesystem::path nf = dir / "NewScene.qscene";
        int c = 1;
        while (std::filesystem::exists(nf))
            nf = dir / ("NewScene" + std::to_string(c++) + ".qscene");

        std::ofstream ofs(nf);
        ofs << "{ \"entities\": [] }\n";
        m_Model.MarkCacheDirty();
    }

    void ContentBrowserActions::CreateNewParticleEffect()
    {
        auto dir = m_Model.CurrentDirectory();
        std::filesystem::path nf = dir / "NewEffect.qparticle";
        int c = 1;
        while (std::filesystem::exists(nf))
            nf = dir / ("NewEffect" + std::to_string(c++) + ".qparticle");

        auto effect = CreateDefaultParticleEffectForPath(nf);
        ParticleEffectSerializer serializer(effect.get());
        serializer.Serialize(nf.string());

        m_ParticleEffectEditor = std::make_shared<ParticleEffectEditor>(m_Ctx);
        m_ParticleEffectEditor->SetCurrentEffect(effect);

        m_Model.MarkCacheDirty();
    }

    void ContentBrowserActions::OpenOnDoubleClick(const ContentBrowserModel::Entry& e)
    {
        if (e.isDir) {
            m_Model.ChangeDirectory(e.absPath);
            return;
        }

        if (e.type == AssetType::TEXTURE) {
            m_TextureViewer = std::make_shared<TextureViewer>(m_Ctx);
            m_TextureViewer->SetTexturePath(WeakCanonical(e.absPath));
            return;
        }

        if (e.type == AssetType::SCRIPT) {
            m_CodeEditor = std::make_shared<CodeEditor>(m_Ctx);
            if (e.extLower == ".lua") m_CodeEditor->SetLanguageDefinition(TextEditor::LanguageDefinition::Lua());
            else if (e.extLower == ".cpp" || e.extLower == ".h")
                m_CodeEditor->SetLanguageDefinition(TextEditor::LanguageDefinition::CPlusPlus());
            m_CodeEditor->LoadFromFile(e.absPath.string());
            return;
        }

        if (e.type == AssetType::PARTICLE) {
            m_ParticleEffectEditor = std::make_shared<ParticleEffectEditor>(m_Ctx);
            auto effect = std::make_shared<ParticleEffect>();
            ParticleEffectSerializer serializer;
            serializer.SetEffect(effect.get());
            serializer.Deserialize(e.absPath.string());
            m_ParticleEffectEditor->SetCurrentEffect(effect);
            return;
        }
    }
}

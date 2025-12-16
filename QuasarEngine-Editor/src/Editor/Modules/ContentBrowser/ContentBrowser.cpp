#include "ContentBrowser.h"

#include <filesystem>
#include <system_error>

#include "ContentBrowserModel.h"
#include "ContentBrowserActions.h"
#include "ContentBrowserView.h"

#include <QuasarEngine/Resources/Texture2D.h>
#include "Editor/Resources/images_data.h"

namespace QuasarEngine
{
    ContentBrowser::ContentBrowser(EditorContext& context)
        : IEditorModule(context)
    {
        const std::filesystem::path baseDir = (std::filesystem::path(context.projectPath) / "Assets").lexically_normal();
        m_Model = std::make_unique<ContentBrowserModel>(baseDir);
        m_Actions = std::make_unique<ContentBrowserActions>(m_Context, *m_Model);
        m_View = std::make_unique<ContentBrowserView>(*m_Model, *m_Actions);

        TextureSpecification spec;

        auto dirIcon = Texture2D::Create(spec);
        dirIcon->LoadFromMemory({ img_texture_dossier, img_texture_dossier_size });

        auto pngIcon = Texture2D::Create(spec);
        pngIcon->LoadFromMemory({ img_texture_png, img_texture_png_size });

        auto jpgIcon = Texture2D::Create(spec);
        jpgIcon->LoadFromMemory({ img_texture_jpg, img_texture_jpg_size });

        auto objIcon = Texture2D::Create(spec);
        objIcon->LoadFromMemory({ img_texture_obj, img_texture_obj_size });

        auto luaIcon = Texture2D::Create(spec);
        luaIcon->LoadFromMemory({ img_texture_lua, img_texture_lua_size });

        auto sceneIcon = Texture2D::Create(spec);
        sceneIcon->LoadFromMemory({ img_texture_scene, img_texture_scene_size });

        auto otherIcon = Texture2D::Create(spec);
        otherIcon->LoadFromMemory({ img_texture_texte, img_texture_texte_size });

        ContentBrowserModel::IconSet icons{};
        icons.directory = dirIcon;
        icons.png = pngIcon;
        icons.jpg = jpgIcon;
        icons.obj = objIcon;
        icons.lua = luaIcon;
        icons.scene = sceneIcon;
        icons.other = otherIcon;

        m_Model->SetIconSet(std::move(icons));

        std::filesystem::path trash = baseDir / ".Trash";
        std::error_code ec;
        std::filesystem::create_directories(trash, ec);
        m_Model->SetTrashDirectory(trash);

        m_Model->ChangeDirectory(baseDir);
    }

    ContentBrowser::~ContentBrowser() = default;

    void ContentBrowser::Update(double dt)
    {
        m_Model->Update(dt);
        m_Actions->Update(dt);
    }

    void ContentBrowser::RenderUI()
    {
        m_Actions->RenderSubEditors();
        m_View->Render();
    }
}

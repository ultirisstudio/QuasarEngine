#include "ContentBrowserPanel.h"

#include <iostream>

#include "imgui/imgui.h"
#include "Editor/Importer/TextureConfigImporter.h"
#include "Editor/Importer/TextureImporter.h"

#include <QuasarEngine/Renderer/Renderer.h>
#include <QuasarEngine/Asset/Asset.h>

#include <QuasarEngine/Core/Logger.h>

#include "Editor/Resources/images_data.h"

namespace QuasarEngine
{
    ContentBrowserPanel::ContentBrowserPanel(const std::string& projectPath, AssetImporter* importer)
        : m_BaseDirectory((std::filesystem::path(projectPath) / "Assets").lexically_normal())
        , m_CurrentDirectory(m_BaseDirectory)
        , m_AssetImporter(importer)
    {
        TextureSpecification spec;

        m_DirectoryIcon = Texture2D::Create(spec);
        m_DirectoryIcon->LoadFromMemory({ img_texture_dossier, img_texture_dossier_size });

        m_FilePNGIcon = Texture2D::Create(spec);
        m_FilePNGIcon->LoadFromMemory({ img_texture_png, img_texture_png_size });

        m_FileJPGIcon = Texture2D::Create(spec);
        m_FileJPGIcon->LoadFromMemory({ img_texture_jpg, img_texture_jpg_size });

        m_FileOBJIcon = Texture2D::Create(spec);
        m_FileOBJIcon->LoadFromMemory({ img_texture_obj, img_texture_obj_size });

        m_FileSceneIcon = Texture2D::Create(spec);
        m_FileSceneIcon->LoadFromMemory({ img_texture_scene, img_texture_scene_size });

        m_FileOtherIcon = Texture2D::Create(spec);
        m_FileOtherIcon->LoadFromMemory({ img_texture_texte, img_texture_texte_size });

        m_FileLuaIcon = Texture2D::Create(spec);
        m_FileLuaIcon->LoadFromMemory({ img_texture_lua, img_texture_lua_size });
    }

	ContentBrowserPanel::~ContentBrowserPanel()
	{
		m_DirectoryIcon.reset();
		m_FilePNGIcon.reset();
		m_FileJPGIcon.reset();
		m_FileOBJIcon.reset();
		m_FileOtherIcon.reset();
	}

	void ContentBrowserPanel::Update()
	{
		if (m_TextureViewerPanel)
		{
			m_TextureViewerPanel->Update();
		}
	}

    void ContentBrowserPanel::OnImGuiRender()
    {
        if (m_TextureViewerPanel)
        {
            if (!m_TextureViewerPanel->IsOpen())
                m_TextureViewerPanel.reset();
            else
                m_TextureViewerPanel->OnImGuiRender();
        }

        if (m_CodeEditor)
        {
            m_CodeEditor->OnImGuiRender();
        }

        ImGui::Begin("Content Browser");

        {
            std::filesystem::path pathIter;
            std::vector<std::filesystem::path> paths;

            for (auto& part : m_CurrentDirectory)
            {
                pathIter /= part;
                paths.push_back(pathIter);
            }

            ImGui::Text("Path: ");
            ImGui::SameLine();

            for (size_t i = 0; i < paths.size(); i++)
            {
                if (i > 0)
                    ImGui::SameLine(0.0f, 5.0f);

                std::string label = paths[i].filename().string();
                if (label.empty())
                    label = "/";
                if (ImGui::SmallButton(label.c_str()))
                {
                    m_CurrentDirectory = paths[i];
                }

                if (i < paths.size() - 1)
                    ImGui::SameLine();
                ImGui::TextUnformatted(">");
                ImGui::SameLine();
            }
        }

        static float backAlpha = 1.0f;
        if (m_CurrentDirectory != std::filesystem::path(m_BaseDirectory))
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 1.0f, backAlpha));
            if (ImGui::Button("<- Back"))
            {
                m_CurrentDirectory = m_CurrentDirectory.parent_path();
                backAlpha = 0.5f;
            }
            ImGui::PopStyleColor();

            backAlpha += 0.05f;
            if (backAlpha > 1.0f)
                backAlpha = 1.0f;
        }

        static char searchBuffer[128] = "";
        ImGui::InputTextWithHint("##search", "Search files...", searchBuffer, IM_ARRAYSIZE(searchBuffer));
        std::string searchFilter = searchBuffer;

        enum class SortMode { NameAsc, NameDesc, DateAsc, DateDesc };
        static SortMode sortMode = SortMode::NameAsc;

        ImGui::SameLine();
        if (ImGui::BeginCombo("Sort By",
            sortMode == SortMode::NameAsc ? "Name Up" :
            sortMode == SortMode::NameDesc ? "Name Down" :
            sortMode == SortMode::DateAsc ? "Date Up" : "Date Down"))
        {
            if (ImGui::Selectable("Name Up")) sortMode = SortMode::NameAsc;
            if (ImGui::Selectable("Name Down")) sortMode = SortMode::NameDesc;
            if (ImGui::Selectable("Date Up")) sortMode = SortMode::DateAsc;
            if (ImGui::Selectable("Date Down")) sortMode = SortMode::DateDesc;
            ImGui::EndCombo();
        }

        static bool listView = false;
        ImGui::SameLine();
        if (ImGui::Button(listView ? "<-" : "->"))
        {
            listView = !listView;
        }

        std::vector<std::filesystem::directory_entry> entries;
        for (auto& dirEntry : std::filesystem::directory_iterator(m_CurrentDirectory))
        {
            std::string filenameStr = dirEntry.path().filename().string();
            if (searchFilter.empty() || filenameStr.find(searchFilter) != std::string::npos)
            {
                entries.push_back(dirEntry);
            }
        }

        auto compareFunc = [&](const std::filesystem::directory_entry& a, const std::filesystem::directory_entry& b)
            {
                switch (sortMode)
                {
                case SortMode::NameAsc:
                    return a.path().filename().string() < b.path().filename().string();
                case SortMode::NameDesc:
                    return a.path().filename().string() > b.path().filename().string();
                case SortMode::DateAsc:
                    return std::filesystem::last_write_time(a) < std::filesystem::last_write_time(b);
                case SortMode::DateDesc:
                    return std::filesystem::last_write_time(a) > std::filesystem::last_write_time(b);
                }
                return false;
            };
        std::sort(entries.begin(), entries.end(), compareFunc);

        float cardThumbnailSize = 96.0f;
        float cardPadding = 8.0f;
        float cardTextHeight = ImGui::GetTextLineHeightWithSpacing() * 2.0f;
        float cardTotalHeight = cardThumbnailSize + cardPadding + cardTextHeight;
        float cardTotalWidth = cardThumbnailSize + cardPadding * 2.0f;
        float panelWidth = ImGui::GetContentRegionAvail().x;
        int columnCount = listView ? 1 : std::max(1, (int)(panelWidth / cardTotalWidth));

        ImGui::Columns(columnCount, nullptr, false);

        for (auto& directoryEntry : entries)
        {
            const auto& path = directoryEntry.path();
            std::string filenameString = path.filename().string();

            if (filenameString.empty())
                filenameString = "unknown";

            std::filesystem::path relativePath(path);
            std::string itemPath = relativePath.generic_string();

            std::string extension = path.extension().string();
            if (extension == ".ultconf")
                continue;

            ImGui::PushID(filenameString.c_str());

            std::shared_ptr<Texture2D> icon;

            AssetType fileType = AssetManager::Instance().getTypeFromExtention(extension);

            if (directoryEntry.is_directory())
            {
                icon = m_DirectoryIcon;
            }
            else if (fileType == AssetType::MESH)
            {
                icon = m_FileOBJIcon;
            }
            else if (fileType == AssetType::TEXTURE)
            {
                const std::filesystem::path absPath = path;

                std::error_code ec{};
                std::string id;
                if (auto rel = std::filesystem::relative(absPath, m_BaseDirectory, ec); !ec)
                    id = "Assets/" + rel.generic_string();
                else
                    id = "Assets/" + absPath.filename().generic_string();

                if (AssetManager::Instance().isAssetLoaded(id))
                {
                    icon = AssetManager::Instance().getAsset<Texture2D>(id);
                }
                else
                {
                    static std::unordered_map<std::string, bool> loadingMap;

                    if (!loadingMap[id])
                    {
                        loadingMap[id] = true;

                        TextureSpecification spec = TextureConfigImporter::ImportTextureConfig(absPath.generic_string());

                        AssetToLoad asset{};
                        asset.id = id;
                        asset.path = absPath.generic_string();
                        asset.type = AssetType::TEXTURE;
                        asset.spec = spec;

                        AssetManager::Instance().loadAsset(asset);
                    }

                    if (AssetManager::Instance().isAssetLoaded(id))
                        icon = AssetManager::Instance().getAsset<Texture2D>(id);
                    else
                        icon = m_FileOtherIcon;
                }
            }
            else if (fileType == AssetType::SCRIPT)
            {
                if (extension == ".lua")
                {
                    icon = m_FileLuaIcon;
                }
                else if (extension == ".cpp" || extension == ".h")
                {
                    icon = m_FileOtherIcon;
                }
                else
                {
                    icon = m_FileOtherIcon;
                }
			}
            else if (fileType == AssetType::SCENE)
            {
                icon = m_FileSceneIcon;
            }
            else
            {
                icon = m_FileOtherIcon;
            }

            if (listView)
            {
                ImGui::Image((ImTextureID)icon->GetHandle(), ImVec2(24, 24));
                ImGui::SameLine();
                ImGui::TextUnformatted(filenameString.c_str());
            }
            else
            {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
                ImGui::ImageButton("##", (ImTextureID)icon->GetHandle(), { cardThumbnailSize, cardThumbnailSize }, { 0, 1 }, { 1, 0 });
                ImGui::PopStyleColor();
            }

            if (ImGui::BeginPopupContextItem())
            {
                if (fileType == AssetType::TEXTURE)
                {
                    if (ImGui::MenuItem("Modify"))
                    {
                        m_TextureViewerPanel = std::make_shared<TextureViewerPanel>(WeakCanonical(relativePath));
                    }
                    ImGui::Separator();
                }
                else if (fileType == AssetType::SCRIPT)
                {
                    if (ImGui::MenuItem("Open in Code Editor"))
                    {
						m_CodeEditor = std::make_shared<CodeEditor>();
                        m_CodeEditor->LoadFromFile(relativePath.string());
					}
                    ImGui::Separator();
                }
                if (ImGui::MenuItem("Delete"))
                {
                    std::cout << "Delete: " << filenameString << std::endl;
                }
                ImGui::EndPopup();
            }

            if (ImGui::BeginDragDropSource())
            {
                const wchar_t* itemPathW = relativePath.c_str();
                ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", itemPathW, (wcslen(itemPathW) + 1) * sizeof(wchar_t));
                ImGui::EndDragDropSource();
            }

            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            {
                if (directoryEntry.is_directory())
                {
                    m_CurrentDirectory /= path.filename();
                }
                else if (fileType == AssetType::TEXTURE)
                {
                    m_TextureViewerPanel = std::make_shared<TextureViewerPanel>(WeakCanonical(relativePath));
                }
                else if (fileType == AssetType::SCRIPT)
                {
                    m_CodeEditor = std::make_shared<CodeEditor>();
                    if (extension == ".lua")
                    {
						m_CodeEditor->SetLanguageDefinition(TextEditor::LanguageDefinition::Lua());
					}
					else if (extension == ".cpp" || extension == ".h")
					{
						m_CodeEditor->SetLanguageDefinition(TextEditor::LanguageDefinition::CPlusPlus());
					}
                    m_CodeEditor->LoadFromFile(relativePath.string());
                }
                else
                {
                    m_CodeEditor = std::make_shared<CodeEditor>();
                    if (extension == ".txt")
                    {
                        m_CodeEditor->SetLanguageDefinition(TextEditor::LanguageDefinition::LanguageDefinition());
                    }
                    m_CodeEditor->LoadFromFile(relativePath.string());
                }
            }

            if (!listView)
            {
                size_t lastindex = filenameString.find_last_of(".");
                std::string fileName = filenameString.substr(0, lastindex);
                if (!fileName.empty())
                {
                    ImGui::TextWrapped(fileName.c_str());
                }
            }

            ImGui::NextColumn();
            ImGui::PopID();
        }

        ImGui::Columns(1);

        if (ImGui::BeginPopupContextWindow("ContentBrowserContext", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
        {
            if (ImGui::MenuItem("New Folder"))
            {
                std::filesystem::path newFolder = m_CurrentDirectory / "New Folder";
                int counter = 1;
                while (std::filesystem::exists(newFolder))
                {
                    newFolder = m_CurrentDirectory / ("New Folder " + std::to_string(counter++));
                }
                std::filesystem::create_directory(newFolder);
            }

            if (ImGui::BeginMenu("New File"))
            {
                if (ImGui::MenuItem("Text File"))
                {
                    std::filesystem::path newFile = m_CurrentDirectory / "NewFile.txt";
                    int counter = 1;
                    while (std::filesystem::exists(newFile))
                    {
                        newFile = m_CurrentDirectory / ("NewFile" + std::to_string(counter++) + ".txt");
                    }
                    std::ofstream ofs(newFile);
                    ofs << "";
                    ofs.close();
                }

                if (ImGui::MenuItem("Lua Script"))
                {
                    std::filesystem::path newFile = m_CurrentDirectory / "NewScript.lua";
                    int counter = 1;
                    while (std::filesystem::exists(newFile))
                    {
                        newFile = m_CurrentDirectory / ("NewScript" + std::to_string(counter++) + ".lua");
                    }
                    std::ofstream ofs(newFile);
                    ofs << "-- Default Lua Script\n"
                        "function OnStart()\n"
                        "    print(\"Hello from Lua!\")\n"
                        "end\n\n"
                        "function OnUpdate(dt)\n"
                        "    -- Your update logic here\n"
                        "end\n";
                    ofs.close();
                }

                ImGui::EndMenu();
            }

            ImGui::EndPopup();
        }

        ImGui::End();
    }
}
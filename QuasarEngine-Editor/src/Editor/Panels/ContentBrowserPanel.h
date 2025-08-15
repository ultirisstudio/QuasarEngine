#pragma once

#include <filesystem>

#include "TextureViewerPanel.h"

#include <QuasarEngine/Resources/Texture2D.h>

#include "Editor/Importer/AssetImporter.h"
#include "Editor/Panels/CodeEditor.h"

namespace QuasarEngine
{
	class ContentBrowserPanel
	{
	public:
		ContentBrowserPanel(const std::string& projectPath, AssetImporter* importer);
		~ContentBrowserPanel();

		void Update();

		void OnImGuiRender();
	private:
		const std::string GetFileExtension(std::filesystem::directory_entry e);

		std::filesystem::path m_BaseDirectory;
		std::filesystem::path m_CurrentDirectory;

		std::shared_ptr<Texture2D> m_DirectoryIcon;
		std::shared_ptr<Texture2D> m_FilePNGIcon;
		std::shared_ptr<Texture2D> m_FileJPGIcon;
		std::shared_ptr<Texture2D> m_FileOBJIcon;
		std::shared_ptr<Texture2D> m_FileLuaIcon;
		std::shared_ptr<Texture2D> m_FileSceneIcon;
		std::shared_ptr<Texture2D> m_FileOtherIcon;

		std::shared_ptr<TextureViewerPanel> m_TextureViewerPanel;
		std::shared_ptr<CodeEditor> m_CodeEditor;

		AssetImporter* m_AssetImporter;

		std::future<std::shared_ptr<Texture2D>> text;
	};
}
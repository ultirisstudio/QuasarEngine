#pragma once

#include <filesystem>
#include <memory>

#include <QuasarEngine/Resources/Texture2D.h>

#include <Editor/Modules/IEditorModule.h>

namespace QuasarEngine
{
	class TextureViewer : public IEditorModule
	{
	public:
		TextureViewer(EditorContext& context);
		~TextureViewer() override;

		void Update(double dt) override;
		void Render() override;
		void RenderUI() override;

		void SetTexturePath(const std::filesystem::path& path);

	private:
		std::string BuildTextureIdFromPath(const std::filesystem::path& absOrRelPath) const;

		static std::filesystem::path FindAssetsRoot(const std::filesystem::path& p);

	private:
		std::filesystem::path m_TexturePath;

		std::shared_ptr<Texture2D> m_Texture;
		TextureSpecification m_Specification;
	};
}
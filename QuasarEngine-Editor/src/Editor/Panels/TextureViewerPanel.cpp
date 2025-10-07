#include "TextureViewerPanel.h"

#include <iostream>
#include <imgui/imgui.h>

#include "../Importer/TextureImporter.h"
#include "../Importer/TextureConfigImporter.h"

#include <QuasarEngine/Renderer/Renderer.h>

namespace QuasarEngine
{
	TextureViewerPanel::TextureViewerPanel(std::filesystem::path path) : m_TexturePath(path)
	{
		
	}

	void TextureViewerPanel::Update()
	{
		if (!m_Texture)
		{
			m_Texture = AssetManager::Instance().getAsset<Texture2D>(m_TexturePath.filename().string());

			if (m_Texture)
			{
				m_Specification = m_Texture->GetSpecification();
			}
		}
	}

	void TextureViewerPanel::OnImGuiRender()
	{
		std::string name = m_TexturePath.filename().string() + " (Texture Viewer)";
		ImGui::Begin(name.c_str());

		if (m_Texture)
		{
			ImGui::Columns(2, 0, true);

			ImGui::Image((ImTextureID)m_Texture->GetHandle(), ImVec2{ 512, 512 }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });

			ImGui::NextColumn();

			ImGui::Text("Width: %d", m_Specification.width); ImGui::SameLine();
			ImGui::Text("Height: %d", m_Specification.height);
			ImGui::Text("Channels: %d", m_Specification.channels);

			ImGui::Checkbox("Alpha", &m_Specification.alpha);
			ImGui::Checkbox("Gamma", &m_Specification.gamma);
			ImGui::Checkbox("Flip Vertically", &m_Specification.flip);

			const char* wrap_items[] = { "Repeat", "Mirrored Repeat", "Clamp to Edge", "Clamp to Border" };
			const char* filter_items[] = { "Nearest", "Linear", "Nearest Mipmap Nearest", "Linear Mipmap Nearest", "Nearest Mipmap Linear", "Linear Mipmap Linear" };

			const char* wrap_r_current_item = TextureUtils::TextureWrapToChar(m_Specification.wrap_r);
			const char* wrap_s_current_item = TextureUtils::TextureWrapToChar(m_Specification.wrap_s);
			const char* wrap_t_current_item = TextureUtils::TextureWrapToChar(m_Specification.wrap_t);

			const char* min_filter_current_item = TextureUtils::TextureFilterToChar(m_Specification.min_filter_param);
			const char* mag_filter_current_item = TextureUtils::TextureFilterToChar(m_Specification.mag_filter_param);

			if (ImGui::BeginCombo("Wrap R", wrap_r_current_item))
			{
				for (int n = 0; n <= 3; n++)
				{
					bool is_selected = (wrap_r_current_item == wrap_items[n]);
					if (ImGui::Selectable(wrap_items[n], is_selected))
					{
						m_Specification.wrap_r = TextureUtils::CharToTextureWrap(wrap_items[n]);
					}
					if (is_selected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}

			if (ImGui::BeginCombo("Wrap S", wrap_s_current_item))
			{
				for (int n = 0; n <= 3; n++)
				{
					bool is_selected = (wrap_s_current_item == wrap_items[n]);
					if (ImGui::Selectable(wrap_items[n], is_selected))
					{
						m_Specification.wrap_s = TextureUtils::CharToTextureWrap(wrap_items[n]);
					}
					if (is_selected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}

			if (ImGui::BeginCombo("Wrap T", wrap_t_current_item))
			{
				for (int n = 0; n <= 3; n++)
				{
					bool is_selected = (wrap_t_current_item == wrap_items[n]);
					if (ImGui::Selectable(wrap_items[n], is_selected))
					{
						m_Specification.wrap_t = TextureUtils::CharToTextureWrap(wrap_items[n]);
					}
					if (is_selected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}

			if (ImGui::BeginCombo("Min Filter", min_filter_current_item))
			{
				for (int n = 0; n <= 5; n++)
				{
					bool is_selected = (min_filter_current_item == filter_items[n]);
					if (ImGui::Selectable(filter_items[n], is_selected))
					{
						m_Specification.min_filter_param = TextureUtils::CharToTextureFilter(filter_items[n]);
					}
					if (is_selected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}

			if (ImGui::BeginCombo("Mag Filter", mag_filter_current_item))
			{
				for (int n = 0; n <= 5; n++)
				{
					bool is_selected = (mag_filter_current_item == filter_items[n]);
					if (ImGui::Selectable(filter_items[n], is_selected))
					{
						m_Specification.mag_filter_param = TextureUtils::CharToTextureFilter(filter_items[n]);
					}
					if (is_selected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}

			if (ImGui::Button("Apply"))
			{
				const std::filesystem::path p = m_TexturePath;
				const std::string id = p.filename().string();

				if (p.extension() == ".qasset")
				{
					TextureImporter::updateTexture(p.generic_string(), m_Specification);

					AssetToLoad asset = TextureImporter::importTexture(p.generic_string());

					if (asset.id.empty())   asset.id = id;
					if (asset.path.empty()) asset.path = p.generic_string();
					asset.type = AssetType::TEXTURE;

					AssetManager::Instance().updateAsset(asset);

					m_Texture = nullptr;
				}
				else
				{
					AssetToLoad asset{};
					asset.id = id;
					asset.path = p.generic_string();
					asset.type = AssetType::TEXTURE;
					asset.spec = m_Specification;

					AssetManager::Instance().updateAsset(asset);

					TextureConfigImporter::ExportTextureConfig(p, m_Specification);

					m_Texture = nullptr;
				}
			}


			if (ImGui::Button("Close"))
			{
				m_IsOpen = false;
			}
		}
		else
		{
			ImGui::Text("Texture not found");

			if (ImGui::Button("Close"))
			{
				m_IsOpen = false;
			}
		}

		ImGui::End();
	}
}
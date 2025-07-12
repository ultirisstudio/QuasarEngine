#pragma once

#include <string>
#include <optional>
#include <unordered_map>
#include <glm/glm.hpp>

#include <QuasarEngine/Resources/Texture2D.h>

namespace QuasarEngine
{
	enum TextureType
	{
		Albedo,
		Normal,
		Metallic,
		Roughness,
		AO
	};

	struct MaterialSpecification
	{
		std::optional<std::string> AlbedoTexture;
		std::optional<std::string> NormalTexture;
		std::optional<std::string> MetallicTexture;
		std::optional<std::string> RoughnessTexture;
		std::optional<std::string> AOTexture;

		glm::vec4 Albedo = glm::vec4(1.0f);

		float Metallic = 0.0f;
		float Roughness = 0.5f;
		float AO = 1.0f;
	};

	class Material
	{
	private:
		MaterialSpecification m_Specification;

		std::unordered_map<TextureType, Texture2D*> m_Textures;

	public:
		Material(const MaterialSpecification& specification);
		~Material();

		void SetTexture(TextureType type, Texture2D* texture);
		void SetTexture(TextureType type, std::string path);
		Texture* GetTexture(TextureType type);

		bool HasTexture(TextureType type);

		const MaterialSpecification& GetSpecification() { return m_Specification; }

		glm::vec4& GetAlbedo() { return m_Specification.Albedo; }

		float& GetMetallic() { return m_Specification.Metallic; }
		float& GetRoughness() { return m_Specification.Roughness; }
		float& GetAO() { return m_Specification.AO; }

		std::optional<std::string> GetTexturePath(TextureType type);

		void ResetTexture(TextureType type);

		static std::shared_ptr<Material> CreateMaterial(const MaterialSpecification& specification);

		uint32_t m_ID;
		uint32_t m_Generation;
	};
}

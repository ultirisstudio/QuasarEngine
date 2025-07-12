#pragma once

#include <memory>

#include <QuasarEngine/Entity/Component.h>
#include <QuasarEngine/Resources/Texture2D.h>
#include <QuasarEngine/Shader/Shader.h>

namespace QuasarEngine
{
	class TerrainComponent : public Component
	{
	private:
		std::shared_ptr<Texture2D> m_HeightMapTexture;
		//std::shared_ptr<Shader> m_Shader;

		std::string m_HeightMapPath;

		bool m_Generated = false;

		int m_MaxTessLevel;

		unsigned int terrainVAO;
	public:
		TerrainComponent();

		bool m_PolygonMode = false;
		int rez = 20;
		int textureScale = 1;
		float heightMult = 16.0f;

		void SetHeightMap(const std::string& path) { m_HeightMapPath = path; }

		std::string GetHeightMapPath() { return m_HeightMapPath; }
		Texture2D& GetHeightMapTexture() { return *m_HeightMapTexture; }

		//Shader& GetShader();

		bool IsGenerated() { return m_Generated; }

		void GenerateTerrain();
		void Draw();
	};
}
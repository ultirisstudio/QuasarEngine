#include "qepch.h"
#include "TerrainComponent.h"

#include <QuasarEngine/Renderer/Renderer.h>
#include <QuasarEngine/Resources/Model.h>

#include <stb_image.h>

namespace QuasarEngine
{
	TerrainComponent::TerrainComponent() : terrainVAO(0)
	{
		/*ShaderFile shaderFiles;
		shaderFiles.vertexShaderFile = "Assets/Shaders/gpuheight.vs";
		shaderFiles.fragmentShaderFile = "Assets/Shaders/gpuheight.fs";
		shaderFiles.tessControlShaderFile = "Assets/Shaders/gpuheight.tcs";
		shaderFiles.tessEvaluationShaderFile = "Assets/Shaders/gpuheight.tes";

		m_Shader = Shader::Create(shaderFiles);*/

		//glGetIntegerv(GL_MAX_TESS_GEN_LEVEL, &m_MaxTessLevel);
		//std::cout << "Max available tess level: " << m_MaxTessLevel << std::endl;

		//glPatchParameteri(GL_PATCH_VERTICES, 4);
	}

	//Shader& TerrainComponent::GetShader()
	//{
		//return *m_Shader;
	//}

	void TerrainComponent::GenerateTerrain()
	{
		m_Generated = true;

		TextureSpecification spec;
		spec.min_filter_param = TextureFilter::LINEAR_MIPMAP_LINEAR;
		spec.mag_filter_param = TextureFilter::LINEAR;

		m_HeightMapTexture = Texture2D::CreateTexture2D(spec);
		m_HeightMapTexture->LoadFromPath(m_HeightMapPath);

		int width, height;
		width = m_HeightMapTexture->GetSpecification().width;
		height = m_HeightMapTexture->GetSpecification().height;

		std::vector<float> vertices;
		for (unsigned i = 0; i <= rez - 1; i++)
		{
			for (unsigned j = 0; j <= rez - 1; j++)
			{
				vertices.push_back(-width / 2.0f + width * i / (float)rez); // v.x
				vertices.push_back(0.0f); // v.y
				vertices.push_back(-height / 2.0f + height * j / (float)rez); // v.z
				vertices.push_back(i / (float)rez); // u
				vertices.push_back(j / (float)rez); // v

				vertices.push_back(-width / 2.0f + width * (i + 1) / (float)rez); // v.x
				vertices.push_back(0.0f); // v.y
				vertices.push_back(-height / 2.0f + height * j / (float)rez); // v.z
				vertices.push_back((i + 1) / (float)rez); // u
				vertices.push_back(j / (float)rez); // v

				vertices.push_back(-width / 2.0f + width * i / (float)rez); // v.x
				vertices.push_back(0.0f); // v.y
				vertices.push_back(-height / 2.0f + height * (j + 1) / (float)rez); // v.z
				vertices.push_back(i / (float)rez); // u
				vertices.push_back((j + 1) / (float)rez); // v

				vertices.push_back(-width / 2.0f + width * (i + 1) / (float)rez); // v.x
				vertices.push_back(0.0f); // v.y
				vertices.push_back(-height / 2.0f + height * (j + 1) / (float)rez); // v.z
				vertices.push_back((i + 1) / (float)rez); // u
				vertices.push_back((j + 1) / (float)rez); // v
			}
		}
		std::cout << "Loaded " << rez * rez << " patches of 4 control points each" << std::endl;
		std::cout << "Processing " << rez * rez * 4 << " vertices in vertex shader" << std::endl;

		/*unsigned int terrainVBO;
		glGenVertexArrays(1, &terrainVAO);
		glBindVertexArray(terrainVAO);

		glGenBuffers(1, &terrainVBO);
		glBindBuffer(GL_ARRAY_BUFFER, terrainVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertices.size(), &vertices[0], GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);
		
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(sizeof(float) * 3));
		glEnableVertexAttribArray(1);

		glBindVertexArray(0);*/
	}
	void TerrainComponent::Draw()
	{
		/*if (!m_Generated)
			return;

		glBindVertexArray(terrainVAO);

		glDrawArrays(GL_PATCHES, 0, 4 * rez * rez);

		glBindVertexArray(0);*/
	}
}
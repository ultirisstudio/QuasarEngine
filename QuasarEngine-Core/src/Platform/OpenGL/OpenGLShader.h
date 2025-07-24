#pragma once

#include "OpenGLBuffer.h"
#include "OpenGLTexture2D.h"

#include <string>
#include <unordered_map>
#include <vector>

#include <QuasarEngine/Shader/Shader.h>
#include <glm/glm.hpp>

namespace QuasarEngine
{
	class OpenGLShader : public Shader
	{
	public:
		OpenGLShader(const ShaderDescription& desc);
		~OpenGLShader() override;

		void Use() override;
		void Unuse() override;
		void Reset() override;

		bool UpdateGlobalState() override;
		bool UpdateObject(Material* material) override;

		bool AcquireResources(Material*) override { return true; }
		void ReleaseResources(Material*) override {}

		void SetUniform(const std::string& name, void* data, size_t size) override;
		void SetTexture(const std::string& name, Texture* texture, SamplerType type) override;

	private:
		void ApplyPipelineStates() override;

		uint32_t m_ID = 0;
		std::unordered_map<std::string, int> m_UniformLocations;
		std::unordered_map<std::string, const ShaderUniformDesc*> m_GlobalUniformMap;
		std::unordered_map<std::string, const ShaderUniformDesc*> m_ObjectUniformMap;

		std::unique_ptr<OpenGLUniformBuffer> m_GlobalUBO;
		std::unique_ptr<OpenGLUniformBuffer> m_ObjectUBO;

		std::vector<uint8_t> m_GlobalUniformData;
		std::vector<uint8_t> m_ObjectUniformData;

		std::unordered_map<std::string, OpenGLTexture2D*> m_ObjectTextures;
		std::unordered_map<std::string, Shader::SamplerType> m_ObjectTextureTypes;

		OpenGLTexture2D* defaultBlueTexture;

		ShaderDescription m_Description;

		std::string ReadFile(const std::string& path);
		uint32_t CompileShader(const std::string& source, uint32_t type);
		void LinkProgram(const std::vector<uint32_t>& shaders);
		void ExtractUniformLocations();
	};
}

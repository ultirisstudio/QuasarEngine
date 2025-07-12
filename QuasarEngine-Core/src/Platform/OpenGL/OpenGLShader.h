#pragma once

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

		bool UpdateGlobalState() override { return true; }
		bool UpdateObject(Material*) override { return true; }

		bool AcquireResources(Material*) override { return true; }
		void ReleaseResources(Material*) override {}

		void SetUniform(const std::string& name, void* data, size_t size) override;
		void SetTexture(const std::string& name, Texture* texture) override {}

	private:
		uint32_t m_ID = 0;
		std::unordered_map<std::string, int> m_UniformLocations;
		std::unordered_map<std::string, const ShaderUniformDesc*> m_GlobalUniformMap;
		std::unordered_map<std::string, const ShaderUniformDesc*> m_ObjectUniformMap;

		ShaderDescription m_Description;

		std::string ReadFile(const std::string& path);
		uint32_t CompileShader(const std::string& source, uint32_t type);
		void LinkProgram(const std::vector<uint32_t>& shaders);
		void ExtractUniformLocations();
	};
}

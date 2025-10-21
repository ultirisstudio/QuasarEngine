#include "qepch.h"

#include "DirectXShader.h"

#include <fstream>
#include <sstream>
#include <glm/gtc/type_ptr.hpp>
#include <QuasarEngine/Core/Logger.h>

namespace QuasarEngine
{
	std::string DirectXShader::ReadFile(const std::string& path)
	{
		std::ifstream file(path);
		if (!file.is_open())
			throw std::runtime_error("Failed to open shader file: " + path);

		std::stringstream ss;
		ss << file.rdbuf();
		return ss.str();
	}

	uint32_t DirectXShader::CompileShader(const std::string& source, uint32_t type)
	{
		uint32_t shader = 0;
		
		return shader;
	}

	void DirectXShader::LinkProgram(const std::vector<uint32_t>& shaders)
	{
		
	}

	void DirectXShader::ExtractUniformLocations()
	{
		
	}

	DirectXShader::DirectXShader(const ShaderDescription& desc)
		: m_Description(desc)
	{
		
	}

	DirectXShader::~DirectXShader()
	{
		
	}

	void DirectXShader::Use()
	{
		ApplyPipelineStates();
	}

	void DirectXShader::Unuse()
	{
		
	}

	void DirectXShader::Reset()
	{
		
	}

	bool DirectXShader::UpdateGlobalState()
	{
		return true;
	}

	bool DirectXShader::UpdateObject(Material* material)
	{
		return true;
	}

	bool DirectXShader::SetUniform(const std::string& name, void* data, size_t size)
	{
		return true;
	}

	bool DirectXShader::SetTexture(const std::string& name, Texture* texture, SamplerType type)
	{
		return true;
	}

	void DirectXShader::ApplyPipelineStates()
	{
		
	}
}

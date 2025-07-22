#include "qepch.h"
#include "OpenGLShader.h"

#include <glad/glad.h>
#include <fstream>
#include <sstream>
#include <glm/gtc/type_ptr.hpp>
#include <QuasarEngine/Core/Logger.h>

namespace QuasarEngine
{
	std::string OpenGLShader::ReadFile(const std::string& path)
	{
		std::ifstream file(path);
		if (!file.is_open())
			throw std::runtime_error("Failed to open shader file: " + path);

		std::stringstream ss;
		ss << file.rdbuf();
		return ss.str();
	}

	uint32_t OpenGLShader::CompileShader(const std::string& source, uint32_t type)
	{
		const char* src = source.c_str();
		uint32_t shader = glCreateShader(type);
		glShaderSource(shader, 1, &src, nullptr);
		glCompileShader(shader);

		int success;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			char log[1024];
			glGetShaderInfoLog(shader, 1024, nullptr, log);
			throw std::runtime_error("Shader compile error: " + std::string(log));
		}
		return shader;
	}

	void OpenGLShader::LinkProgram(const std::vector<uint32_t>& shaders)
	{
		m_ID = glCreateProgram();
		for (uint32_t shader : shaders)
			glAttachShader(m_ID, shader);

		glLinkProgram(m_ID);

		int success;
		glGetProgramiv(m_ID, GL_LINK_STATUS, &success);
		if (!success)
		{
			char log[1024];
			glGetProgramInfoLog(m_ID, 1024, nullptr, log);
			throw std::runtime_error("Shader link error: " + std::string(log));
		}

		for (uint32_t shader : shaders)
		{
			glDetachShader(m_ID, shader);
			glDeleteShader(shader);
		}
	}

	void OpenGLShader::ExtractUniformLocations()
	{
		int count = 0;
		glGetProgramiv(m_ID, GL_ACTIVE_UNIFORMS, &count);

		char name[128];
		for (int i = 0; i < count; ++i)
		{
			GLsizei length;
			GLint size;
			GLenum type;
			glGetActiveUniform(m_ID, i, sizeof(name), &length, &size, &type, name);
			int location = glGetUniformLocation(m_ID, name);
			m_UniformLocations[name] = location;

			std::cout << name << "\n";
		}
	}

	OpenGLShader::OpenGLShader(const ShaderDescription& desc)
		: m_Description(desc)
	{
		for (const auto& uniform : m_Description.globalUniforms)
			m_GlobalUniformMap[uniform.name] = &uniform;

		for (const auto& uniform : m_Description.objectUniforms)
			m_ObjectUniformMap[uniform.name] = &uniform;

		std::vector<uint32_t> compiledShaders;

		for (const auto& module : desc.modules)
		{
			std::string source = ReadFile(module.path);

			GLenum glStage = 0;
			switch (module.stage)
			{
			case ShaderStageType::Vertex:      glStage = GL_VERTEX_SHADER; break;
			case ShaderStageType::Fragment:    glStage = GL_FRAGMENT_SHADER; break;
			case ShaderStageType::Geometry:    glStage = GL_GEOMETRY_SHADER; break;
			case ShaderStageType::Compute:     glStage = GL_COMPUTE_SHADER; break;
			case ShaderStageType::TessControl: glStage = GL_TESS_CONTROL_SHADER; break;
			case ShaderStageType::TessEval:    glStage = GL_TESS_EVALUATION_SHADER; break;
			default: continue;
			}

			try
			{
				uint32_t shader = CompileShader(source, glStage);
				compiledShaders.push_back(shader);
			}
			catch (const std::exception& e)
			{
				std::cerr << e.what() << std::endl << std::flush;
			}
		}

		LinkProgram(compiledShaders);
		ExtractUniformLocations();
	}

	OpenGLShader::~OpenGLShader()
	{
		glDeleteProgram(m_ID);
	}

	void OpenGLShader::Use()
	{
		glUseProgram(m_ID);
	}

	void OpenGLShader::Unuse()
	{
		glUseProgram(0);
	}

	void OpenGLShader::Reset()
	{
		
	}

	void OpenGLShader::SetUniform(const std::string& name, void* data, size_t size)
	{
		auto it = m_UniformLocations.find(name);
		if (it == m_UniformLocations.end())
		{
			Q_WARNING("Uniform " + name + " not found in active shader program");
			return;
		}

		int location = it->second;

		const ShaderUniformDesc* desc = nullptr;
		auto globalIt = m_GlobalUniformMap.find(name);
		if (globalIt != m_GlobalUniformMap.end())
			desc = globalIt->second;
		else
		{
			auto objectIt = m_ObjectUniformMap.find(name);
			if (objectIt != m_ObjectUniformMap.end())
				desc = objectIt->second;
		}

		if (!desc)
		{
			Q_ERROR("Uniform " + name + " is not described in shader description");
			return;
		}

		if (desc->size != size)
		{
			Q_ERROR("Uniform " + name + " size mismatch (expected " + std::to_string(desc->size) + ", got " + std::to_string(size) + ")");
			return;
		}

		switch (desc->type)
		{
		case ShaderUniformType::Float:
			glUniform1f(location, *reinterpret_cast<float*>(data));
			break;
		case ShaderUniformType::Vec2:
			glUniform2fv(location, 1, glm::value_ptr(*reinterpret_cast<glm::vec2*>(data)));
			break;
		case ShaderUniformType::Vec3:
			glUniform3fv(location, 1, glm::value_ptr(*reinterpret_cast<glm::vec3*>(data)));
			break;
		case ShaderUniformType::Vec4:
			glUniform4fv(location, 1, glm::value_ptr(*reinterpret_cast<glm::vec4*>(data)));
			break;
		case ShaderUniformType::Int:
			glUniform1i(location, *reinterpret_cast<int*>(data));
			break;
		case ShaderUniformType::UInt:
			glUniform1ui(location, *reinterpret_cast<uint32_t*>(data));
			break;
		case ShaderUniformType::Mat3:
			glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(*reinterpret_cast<glm::mat3*>(data)));
			break;
		case ShaderUniformType::Mat4:
			glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(*reinterpret_cast<glm::mat4*>(data)));
			break;
		default:
			Q_ERROR("Unsupported uniform type for '%s'", name.c_str());
			break;
		}
	}
}

#include "qepch.h"
#include "OpenGLShader.h"

#include <glad/glad.h>
#include <fstream>
#include <sstream>
#include <glm/gtc/type_ptr.hpp>
#include <QuasarEngine/Core/Logger.h>

namespace QuasarEngine
{
	static GLenum DepthFuncToGL(Shader::DepthFunc func)
	{
		switch (func)
		{
		case Shader::DepthFunc::Never:        return GL_NEVER;
		case Shader::DepthFunc::Less:         return GL_LESS;
		case Shader::DepthFunc::Equal:        return GL_EQUAL;
		case Shader::DepthFunc::LessOrEqual:    return GL_LEQUAL;
		case Shader::DepthFunc::Greater:      return GL_GREATER;
		case Shader::DepthFunc::NotEqual:     return GL_NOTEQUAL;
		case Shader::DepthFunc::GreaterOrEqual: return GL_GEQUAL;
		case Shader::DepthFunc::Always:       return GL_ALWAYS;
		default:                      return GL_LESS;
		}
	}

	static GLenum SamplerTypeToGL(Shader::SamplerType type)
	{
		switch (type)
		{
		case Shader::SamplerType::Sampler2D:        return GL_TEXTURE_2D;
		case Shader::SamplerType::SamplerCube:		return GL_TEXTURE_CUBE_MAP;
		default:                      return GL_TEXTURE_2D;
		}
	}

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

		GLint numBlocks = 0;
		glGetProgramiv(m_ID, GL_ACTIVE_UNIFORM_BLOCKS, &numBlocks);
		char bname[128];

		for (int i = 0; i < numBlocks; ++i)
		{
			GLsizei len = 0;
			glGetActiveUniformBlockName(m_ID, i, sizeof(bname), &len, bname);
			std::cout << "Uniform Block #" << i << ": " << bname << std::endl;
		}
	}

	OpenGLShader::OpenGLShader(const ShaderDescription& desc)
		: m_Description(desc)
	{
		size_t globalSize = 0;
		for (const auto& uniform : m_Description.globalUniforms)
			globalSize = std::max(globalSize, uniform.offset + uniform.size);
		m_GlobalUniformData.resize(globalSize);
		m_GlobalUBO = std::make_unique<OpenGLUniformBuffer>(globalSize, 0);

		size_t objectSize = 0;
		for (const auto& uniform : m_Description.objectUniforms)
			objectSize = std::max(objectSize, uniform.offset + uniform.size);
		m_ObjectUniformData.resize(objectSize);
		m_ObjectUBO = std::make_unique<OpenGLUniformBuffer>(objectSize, 1);

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

		try
		{
			LinkProgram(compiledShaders);
		}
		catch (const std::exception& e)
		{
			Q_ERROR("Erreur lors du linkage du shader : " + std::string(e.what()));
			throw;
		}

		ExtractUniformLocations();

		TextureSpecification spec;
		spec.width = 2;
		spec.height = 2;
		spec.format = TextureFormat::RGBA;
		spec.internal_format = TextureFormat::RGBA;
		spec.compressed = false;
		spec.alpha = true;
		spec.flip = false;
		spec.wrap_s = TextureWrap::REPEAT;
		spec.wrap_t = TextureWrap::REPEAT;
		spec.wrap_r = TextureWrap::REPEAT;
		spec.min_filter_param = TextureFilter::NEAREST;
		spec.mag_filter_param = TextureFilter::NEAREST;

		std::vector<unsigned char> bluePixels(4 * spec.width * spec.height, 0);
		for (int i = 0; i < spec.width * spec.height; ++i) {
			bluePixels[i * 4 + 0] = 255;
			bluePixels[i * 4 + 1] = 0;
			bluePixels[i * 4 + 2] = 0;
			bluePixels[i * 4 + 3] = 255;
		}
		defaultBlueTexture = new OpenGLTexture2D(spec);
		defaultBlueTexture->LoadFromData(bluePixels.data(), bluePixels.size());
	}

	OpenGLShader::~OpenGLShader()
	{
		glDeleteProgram(m_ID);
	}

	void OpenGLShader::Use()
	{
		glUseProgram(m_ID);

		ApplyPipelineStates();
	}

	void OpenGLShader::Unuse()
	{
		glUseProgram(0);
	}

	void OpenGLShader::Reset()
	{
		
	}

	bool OpenGLShader::UpdateGlobalState()
	{
		m_GlobalUBO->SetData(m_GlobalUniformData.data(), m_GlobalUniformData.size());
		m_GlobalUBO->BindToShader(m_ID, "global_uniform_object");

		return true;
	}

	bool OpenGLShader::UpdateObject(Material* material)
	{
		m_ObjectUBO->SetData(m_ObjectUniformData.data(), m_ObjectUniformData.size());
		m_ObjectUBO->BindToShader(m_ID, "local_uniform_object");

		for (const auto& samplerDesc : m_Description.samplers)
		{
			OpenGLTexture2D* tex = defaultBlueTexture;

			auto it = m_ObjectTextures.find(samplerDesc.name);
			if (it != m_ObjectTextures.end() && it->second)
				tex = it->second;

			if (!tex || tex->GetHandle() == 0)
			{
				Q_ERROR("Invalid texture for sampler " + samplerDesc.name);
				continue;
			}

			GLenum type = GL_TEXTURE_2D;

			auto it2 = m_ObjectTextureTypes.find(samplerDesc.name);
			if (it2 != m_ObjectTextureTypes.end())
				type = SamplerTypeToGL(it2->second);

			glActiveTexture(GL_TEXTURE0 + samplerDesc.binding);
			glBindTexture(type, reinterpret_cast<GLuint>(tex->GetHandle()));

			auto loc = m_UniformLocations.find(samplerDesc.name);
			if (loc != m_UniformLocations.end())
				glUniform1i(loc->second, samplerDesc.binding);
		}

		return true;
	}

	void OpenGLShader::SetUniform(const std::string& name, void* data, size_t size)
	{
		auto gIt = m_GlobalUniformMap.find(name);
		if (gIt != m_GlobalUniformMap.end())
		{
			const auto* desc = gIt->second;
			if (desc->offset + desc->size <= m_GlobalUniformData.size())
				std::memcpy(m_GlobalUniformData.data() + desc->offset, data, desc->size);
			return;
		}

		auto oIt = m_ObjectUniformMap.find(name);
		if (oIt != m_ObjectUniformMap.end())
		{
			const auto* desc = oIt->second;
			if (desc->offset + desc->size <= m_ObjectUniformData.size())
				std::memcpy(m_ObjectUniformData.data() + desc->offset, data, desc->size);
			return;
		}

		Q_WARNING("Uniform non trouvé : " + name);
	}

	void OpenGLShader::SetTexture(const std::string& name, Texture* texture, SamplerType type)
	{
		auto it = std::find_if(
			m_Description.samplers.begin(), m_Description.samplers.end(),
			[&](const ShaderSamplerDesc& desc) { return desc.name == name; });

		if (it == m_Description.samplers.end())
		{
			Q_ERROR("Sampler '%s' not found in shader description!", name.c_str());
			return;
		}

		m_ObjectTextures[name] = texture ? dynamic_cast<OpenGLTexture2D*>(texture) : defaultBlueTexture;
		m_ObjectTextureTypes[name] = type;
	}

	void OpenGLShader::ApplyPipelineStates()
	{
		if (m_Description.depthTestEnable)
			glEnable(GL_DEPTH_TEST);
		else
			glDisable(GL_DEPTH_TEST);

		glDepthMask(m_Description.depthWriteEnable ? GL_TRUE : GL_FALSE);
		glDepthFunc(DepthFuncToGL(m_Description.depthFunc));

		if (m_Description.cullMode == CullMode::None)
			glDisable(GL_CULL_FACE);
		else
		{
			glEnable(GL_CULL_FACE);
			glCullFace(m_Description.cullMode == CullMode::Back ? GL_BACK : GL_FRONT);
		}

		glPolygonMode(GL_FRONT_AND_BACK, m_Description.fillMode == FillMode::Wireframe ? GL_LINE : GL_FILL);

		if (m_Description.blendMode != BlendMode::None)
		{
			glEnable(GL_BLEND);
			switch (m_Description.blendMode)
			{
			case BlendMode::AlphaBlend:
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				break;
			case BlendMode::Additive:
				glBlendFunc(GL_ONE, GL_ONE);
				break;
			case BlendMode::Multiply:
				glBlendFunc(GL_DST_COLOR, GL_ZERO);
				break;
			default:
				break;
			}
		}
		else
		{
			glDisable(GL_BLEND);
		}
	}
}

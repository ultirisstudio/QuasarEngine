#include "qepch.h"

#include "VulkanContext.h"
#include "VulkanDevice.h"
#include "VulkanImage.h"
#include "VulkanBuffer.h"
#include "VulkanCommandBuffer.h"
#include "VulkanBasicSkybox.h"

#include <numeric>

#include <stb_image.h>

#include <backends/imgui_impl_vulkan.h>

#include <QuasarEngine/Shader/Shader.h>

namespace QuasarEngine
{
	VulkanBasicSkybox::VulkanBasicSkybox()
	{
		Shader::ShaderDescription desc;

		desc.modules = {
			Shader::ShaderModuleInfo{
				Shader::ShaderStageType::Vertex,
				"Assets/Shaders/vk/spv/basic_skybox.vert.spv",
				{
					{0, Shader::ShaderIOType::Vec3, "inPosition", true, ""},
					{1, Shader::ShaderIOType::Vec3, "inNormal",   true, ""},
					{2, Shader::ShaderIOType::Vec2, "inTexCoord", true, ""}
				}
			},
			Shader::ShaderModuleInfo{
				Shader::ShaderStageType::Fragment,
				"Assets/Shaders/vk/spv/basic_skybox.frag.spv",
				{
					{0, Shader::ShaderIOType::Vec3, "inTexCoord", true, ""}
				}
			}
		};

		desc.globalUniforms = {
			{"view",  Shader::ShaderUniformType::Mat4, sizeof(glm::mat4), 0, 0, 0,Shader::StageToBit(Shader::ShaderStageType::Vertex)},
			{"projection",  Shader::ShaderUniformType::Mat4, sizeof(glm::mat4),   sizeof(glm::mat4), 0, 0, Shader::StageToBit(Shader::ShaderStageType::Vertex)}
		};

		desc.objectUniforms = {
			{"model",			Shader::ShaderUniformType::Mat4, sizeof(glm::mat4), 0, 1, 0, Shader::StageToBit(Shader::ShaderStageType::Fragment) }
		};

		desc.samplers = {
			{"skybox", 1, 1, Shader::StageToBit(Shader::ShaderStageType::Fragment)}
		};

		desc.blendMode = Shader::BlendMode::None;
		desc.cullMode = Shader::CullMode::None;
		desc.fillMode = Shader::FillMode::Solid;
		desc.depthTestEnable = true;
		desc.depthWriteEnable = false;
		desc.depthFunc = Shader::DepthFunc::LessOrEqual;
		desc.topology = Shader::PrimitiveTopology::TriangleList;
		desc.enableDynamicViewport = true;
		desc.enableDynamicScissor = true;
		desc.enableDynamicLineWidth = false;

		shader = Shader::Create(desc);

		MaterialSpecification materialSpec;
		material = Material::CreateMaterial(materialSpec);

		shader->AcquireResources(material.get());

		material->m_Generation++;

		TextureSpecification textureSpec;
		textureSpec.width = 2048;
		textureSpec.height = 2048;
		texture = std::make_shared<VulkanTexture2D>(textureSpec);

		std::vector<float> cubeVertices = {
			-1,  1, -1,   0,  0, -1,   0, 1,
			-1, -1, -1,   0,  0, -1,   0, 0,
			 1, -1, -1,   0,  0, -1,   1, 0,
			 1, -1, -1,   0,  0, -1,   1, 0,
			 1,  1, -1,   0,  0, -1,   1, 1,
			-1,  1, -1,   0,  0, -1,   0, 1,

			-1, -1,  1,   0,  0,  1,   0, 0,
			-1,  1,  1,   0,  0,  1,   0, 1,
			 1,  1,  1,   0,  0,  1,   1, 1,
			 1,  1,  1,   0,  0,  1,   1, 1,
			 1, -1,  1,   0,  0,  1,   1, 0,
			-1, -1,  1,   0,  0,  1,   0, 0,

			-1,  1,  1,  -1,  0,  0,   1, 1,
			-1,  1, -1,  -1,  0,  0,   0, 1,
			-1, -1, -1,  -1,  0,  0,   0, 0,
			-1, -1, -1,  -1,  0,  0,   0, 0,
			-1, -1,  1,  -1,  0,  0,   1, 0,
			-1,  1,  1,  -1,  0,  0,   1, 1,

			 1,  1,  1,   1,  0,  0,   1, 1,
			 1, -1,  1,   1,  0,  0,   1, 0,
			 1, -1, -1,   1,  0,  0,   0, 0,
			 1, -1, -1,   1,  0,  0,   0, 0,
			 1,  1, -1,   1,  0,  0,   0, 1,
			 1,  1,  1,   1,  0,  0,   1, 1,

			-1, -1, -1,   0, -1,  0,   0, 1,
			 1, -1, -1,   0, -1,  0,   1, 1,
			 1, -1,  1,   0, -1,  0,   1, 0,
			 1, -1,  1,   0, -1,  0,   1, 0,
			-1, -1,  1,   0, -1,  0,   0, 0,
			-1, -1, -1,   0, -1,  0,   0, 1,

			-1,  1, -1,   0,  1,  0,   0, 1,
			-1,  1,  1,   0,  1,  0,   0, 0,
			 1,  1,  1,   0,  1,  0,   1, 0,
			 1,  1,  1,   0,  1,  0,   1, 0,
			 1,  1, -1,   0,  1,  0,   1, 1,
			-1,  1, -1,   0,  1,  0,   0, 1
		};

		std::vector<uint32_t> cubeIndices(36);
		std::iota(cubeIndices.begin(), cubeIndices.end(), 0);

		cubeMesh = std::make_unique<Mesh>(cubeVertices, cubeIndices, std::nullopt, DrawMode::TRIANGLES);
	}

	VulkanBasicSkybox::~VulkanBasicSkybox()
	{
		shader->ReleaseResources(material.get());

		texture.reset();
		material.reset();
		cubeMesh.reset();
		shader.reset();
	}

	void VulkanBasicSkybox::Bind()
	{
		shader->Use();
	}

	void VulkanBasicSkybox::Unbind()
	{
		shader->Unuse();
	}

	void VulkanBasicSkybox::Draw()
	{
		cubeMesh->draw();
	}

	void VulkanBasicSkybox::LoadCubemap(const std::array<std::string, 6>& faces)
	{
		int width, height, channels;
		VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;

		uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(2048))) + 1;

		const int desiredChannels = 4;
		const size_t faceSize = [&]()
			{
				stbi_info(faces[0].c_str(), &width, &height, &channels);
				return width * height * desiredChannels;
			}();

			const size_t totalSize = faceSize * 6;
			std::vector<unsigned char> buffer(totalSize);

			for (size_t i = 0; i < 6; ++i)
			{
				int w, h, ch;
				unsigned char* data = stbi_load(faces[i].c_str(), &w, &h, &ch, desiredChannels);
				memcpy(buffer.data() + i * faceSize, data, faceSize);
				stbi_image_free(data);
			}

			VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			VkMemoryPropertyFlags memory_prop_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

			std::unique_ptr<VulkanBuffer> staging = std::make_unique<VulkanBuffer>(
				VulkanContext::Context.device->device,
				VulkanContext::Context.device->physicalDevice,
				totalSize,
				usage,
				memory_prop_flags,
				true
			);

			staging->LoadData(0, totalSize, 0, buffer.data());

			texture->image = std::make_unique<VulkanImage>(
				VK_IMAGE_TYPE_2D,
				VK_IMAGE_VIEW_TYPE_CUBE,
				2048,
				2048,
				VK_FORMAT_R8G8B8A8_SRGB,
				VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				true,
				VK_IMAGE_ASPECT_COLOR_BIT,
				6,
				VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
				mipLevels
			);

			VkDevice device = VulkanContext::Context.device->device;
			VkCommandPool pool = VulkanContext::Context.device->graphicsCommandPool;
			VkQueue queue = VulkanContext::Context.device->graphicsQueue;

			std::unique_ptr<VulkanCommandBuffer> temp_buffer = std::make_unique<VulkanCommandBuffer>(device, pool);

			temp_buffer->AllocateAndBeginSingleUse();

			texture->image->ImageTransitionLayout(temp_buffer->handle, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, mipLevels, 0, 6);
			texture->image->CopyFromBuffer(temp_buffer->handle, staging->handle, 6);
			//texture->image->ImageTransitionLayout(temp_buffer->handle, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 1, 0, 6);
			texture->image->GenerateMipmaps(temp_buffer->handle, format, 2048, 2048, mipLevels, 6);
			
			temp_buffer->EndSingleUse(queue);

			VkSamplerCreateInfo info = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
			info.magFilter = VK_FILTER_LINEAR;
			info.minFilter = VK_FILTER_LINEAR;
			info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			info.anisotropyEnable = VK_FALSE;
			info.unnormalizedCoordinates = VK_FALSE;
			vkCreateSampler(VulkanContext::Context.device->device, &info, VulkanContext::Context.allocator->GetCallbacks(), &texture->sampler);

			material->SetTexture(TextureType::Albedo, texture.get());

			texture->generation++;
	}

	Shader* VulkanBasicSkybox::GetShader()
	{
		return shader.get();
	}

	Texture2D* VulkanBasicSkybox::GetTexture()
	{
		return texture.get();
	}

	Material* VulkanBasicSkybox::GetMaterial()
	{
		return material.get();
	}
}

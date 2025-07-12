#include "qepch.h"

#include "VulkanHDRSkybox.h"

#include <numeric>

#include "VulkanTexture2D.h"
#include "VulkanShader.h"

#include <QuasarEngine/Resources/Texture2D.h>
#include <QuasarEngine/Resources/Mesh.h>

#include "VulkanContext.h"
#include "VulkanCommandBuffer.h"
#include "VulkanDevice.h"
#include "VulkanBuffer.h"
#include "VulkanImage.h"

#include <stb_image.h>

namespace QuasarEngine
{
	VulkanHDRSkybox::VulkanHDRSkybox()
	{
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

	VulkanHDRSkybox::~VulkanHDRSkybox()
	{
	}

	void VulkanHDRSkybox::LoadFromEquirectangularHDR(const std::string& path)
	{
        /*int width, height, channels;
        float* data = stbi_loadf(path.c_str(), &width, &height, &channels, 0);
        if (!data)
        {
            Q_ERROR("Failed to load HDR image: %s", path.c_str());
            return;
        }

        size_t pixelSize = 3 * sizeof(float);
        size_t imageSize = width * height * pixelSize;

        std::unique_ptr<VulkanBuffer> staging = std::make_unique<VulkanBuffer>(
            VulkanContext::Context.device->device,
            VulkanContext::Context.device->physicalDevice,
            imageSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            true
        );

        staging->LoadData(0, imageSize, 0, data);

        stbi_image_free(data);

        hdrTexture = std::make_shared<VulkanTexture2D>(TextureSpecification{
            .width = (uint32_t)width,
            .height = (uint32_t)height,
            .format = TextureFormat::RGB,
            .internal_format = TextureFormat::RGB,
            .compressed = false,
            .alpha = false,
            .flip = false,
            .wrap_s = TextureWrap::CLAMP_TO_EDGE,
            .wrap_t = TextureWrap::CLAMP_TO_EDGE,
            .wrap_r = TextureWrap::CLAMP_TO_EDGE,
            .min_filter_param = TextureFilter::LINEAR,
            .mag_filter_param = TextureFilter::LINEAR
            });

        hdrTexture->image = std::make_unique<VulkanImage>(
            VK_IMAGE_TYPE_2D,
            VK_IMAGE_VIEW_TYPE_2D,
            width,
            height,
            VK_FORMAT_R32G32B32_SFLOAT,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            true,
            VK_IMAGE_ASPECT_COLOR_BIT,
            1,
            0
        );

        VkDevice device = VulkanContext::Context.device->device;
        VkCommandPool pool = VulkanContext::Context.device->graphicsCommandPool;
        VkQueue queue = VulkanContext::Context.device->graphicsQueue;

        std::unique_ptr<VulkanCommandBuffer> temp_buffer = std::make_unique<VulkanCommandBuffer>(device, pool);
        temp_buffer->AllocateAndBeginSingleUse();

        hdrTexture->image->ImageTransitionLayout(temp_buffer->handle, VK_FORMAT_R32G32B32_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1);
        hdrTexture->image->CopyFromBuffer(temp_buffer->handle, staging->handle, 1);
        hdrTexture->image->ImageTransitionLayout(temp_buffer->handle, VK_FORMAT_R32G32B32_SFLOAT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);

        temp_buffer->EndSingleUse(queue);

        VkSamplerCreateInfo samplerInfo = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        vkCreateSampler(device, &samplerInfo, VulkanContext::Context.allocator->GetCallbacks(), &hdrTexture->sampler);

        hdrTexture->generation++;*/
	}

	void VulkanHDRSkybox::Draw(const glm::mat4& view, const glm::mat4& projection)
	{
	}

	Texture2D* VulkanHDRSkybox::GetEnvironmentCubemap()
	{
		return nullptr;
	}

	Texture2D* VulkanHDRSkybox::GetIrradianceMap()
	{
		return nullptr;
	}

	Texture2D* VulkanHDRSkybox::GetPrefilterMap()
	{
		return nullptr;
	}

	Texture2D* VulkanHDRSkybox::GetBRDFLUT()
	{
		return nullptr;
	}
}

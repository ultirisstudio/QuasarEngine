#include "qepch.h"

#include "DirectXTexture2D.h"

#include <stb_image.h>

#include <QuasarEngine/File/FileUtils.h>
#include <QuasarEngine/Core/Logger.h>

#include "DirectXContext.h"

namespace QuasarEngine
{
	DirectXTexture2D::DirectXTexture2D(const TextureSpecification& specification)
		: Texture2D(specification)
	{

	}

	DirectXTexture2D::~DirectXTexture2D()
	{
		
	}

	void DirectXTexture2D::LoadFromPath(const std::string& path)
	{
		std::vector<unsigned char> buffer;
		std::ifstream file(path, std::ios::binary);
		if (file)
		{
			file.seekg(0, std::ios::end);
			buffer.resize(file.tellg());
			file.seekg(0, std::ios::beg);
			file.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
		}
		else
		{
			Q_ERROR("Failed to load texture from path: " + path);
			return;
		}

		LoadFromMemory(buffer.data(), buffer.size());
	}

	void DirectXTexture2D::LoadFromMemory(unsigned char* image_data, size_t size)
	{
		if (m_Specification.compressed)
		{
			if (m_Specification.flip)
				stbi_set_flip_vertically_on_load(true);
			else
				stbi_set_flip_vertically_on_load(false);

			int width, height, channels;
			unsigned char* data = stbi_load_from_memory(
				image_data, static_cast<int>(size),
				&width, &height, &channels,
				(m_Specification.alpha ? 4 : 4)
			);

			if (!data)
			{
				Q_ERROR(std::string("Failed to decode image from memory: ") + stbi_failure_reason());
				return;
			}

			m_Specification.width = width;
			m_Specification.height = height;
			m_Specification.channels = channels;

			LoadFromData(data, width * height * (m_Specification.alpha ? 4 : 4));

			stbi_image_free(data);
		}
		else
		{
			LoadFromData(image_data, m_Specification.width * m_Specification.height * 4);
		}
	}

	void DirectXTexture2D::LoadFromData(unsigned char* data, size_t)
	{
		auto& dx = DirectXContext::Context;

		D3D11_TEXTURE2D_DESC desc = {};
		desc.Width = m_Specification.width;
		desc.Height = m_Specification.height;
		desc.MipLevels = m_Specification.mipmap ? 1 : 1;
		desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		if (m_Specification.mipmap)
		{
			desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
			desc.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
		}

		desc.MiscFlags = m_Specification.mipmap ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0;

		D3D11_SUBRESOURCE_DATA initData = {};
		initData.pSysMem = data;
		initData.SysMemPitch = m_Specification.width * (m_Specification.alpha ? 4 : 4);

		HRESULT hr = dx.device->CreateTexture2D(&desc, &initData, m_Texture.GetAddressOf());
		if (FAILED(hr))
		{
			char buf[128];
			sprintf_s(buf, "Failed to create DirectX Texture2D. HRESULT = 0x%08X", hr);
			Q_ERROR(buf);
			return;
		}

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = desc.Format;
		srvDesc.ViewDimension = (desc.SampleDesc.Count > 1) ? D3D11_SRV_DIMENSION_TEXTURE2DMS : D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = -1;

		hr = dx.device->CreateShaderResourceView(m_Texture.Get(), &srvDesc, m_SRV.GetAddressOf());
		if (FAILED(hr))
		{
			Q_ERROR("Failed to create ShaderResourceView for texture");
			return;
		}

		if (m_Specification.mipmap)
		{
			dx.deviceContext->GenerateMips(m_SRV.Get());
		}
	}

	void DirectXTexture2D::Bind(int index) const
	{
		auto& dx = DirectXContext::Context;
		dx.deviceContext->PSSetShaderResources(index, 1, m_SRV.GetAddressOf());
	}

	void DirectXTexture2D::Unbind() const
	{
		auto& dx = DirectXContext::Context;
		ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
		dx.deviceContext->PSSetShaderResources(0, 1, nullSRV);
	}
}

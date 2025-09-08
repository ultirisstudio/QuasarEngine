#pragma once

#include <QuasarEngine/Resources/Texture2D.h>

#include <wrl/client.h>
#include <d3d11.h>

namespace QuasarEngine
{
	class DirectXTexture2D : public Texture2D
	{
	public:
		DirectXTexture2D(const TextureSpecification& specification);
		~DirectXTexture2D() override;

		void LoadFromPath(const std::string& path) override;
		void LoadFromMemory(unsigned char* image_data, size_t size) override;
		void LoadFromData(unsigned char* image_data, size_t size) override;

		void* GetHandle() const override { return reinterpret_cast<void*>(m_SRV.Get()); }

		void Bind(int index) const override;
		void Unbind() const override;

	private:
		Microsoft::WRL::ComPtr<ID3D11Texture2D> m_Texture;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_SRV;
	};
}
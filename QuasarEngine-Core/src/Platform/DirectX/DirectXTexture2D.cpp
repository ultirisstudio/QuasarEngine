#include "qepch.h"
#include "DirectXTexture2D.h"

namespace QuasarEngine
{
	DirectXTexture2D::DirectXTexture2D(const TextureSpecification& specification)
		: Texture2D(specification) {
	}

	DirectXTexture2D::~DirectXTexture2D() = default;

	bool DirectXTexture2D::LoadFromPath(const std::string&) { return false; }
	bool DirectXTexture2D::LoadFromMemory(ByteView) { return false; }
	bool DirectXTexture2D::LoadFromData(ByteView) { return false; }

	void DirectXTexture2D::Bind(int) const {}
	void DirectXTexture2D::Unbind() const {}
}
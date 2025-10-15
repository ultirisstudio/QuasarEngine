#include "qepch.h"

#include "DirectXTextureArray.h"

namespace QuasarEngine
{
	DirectXTextureArray::DirectXTextureArray(const TextureSpecification& specification)
		: TextureArray(specification) {
	}

	DirectXTextureArray::~DirectXTextureArray() = default;

	bool DirectXTextureArray::LoadFromPath(const std::string&) { return false; }
	bool DirectXTextureArray::LoadFromMemory(ByteView) { return false; }
	bool DirectXTextureArray::LoadFromData(ByteView) { return false; }

	void DirectXTextureArray::Bind(int) const {}
	void DirectXTextureArray::Unbind() const {}
}
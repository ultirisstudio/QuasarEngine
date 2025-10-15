#include "qepch.h"

#include "DirectXTextureCubeMap.h"

namespace QuasarEngine
{
	DirectXTextureCubeMap::DirectXTextureCubeMap(const TextureSpecification& specification)
		: TextureCubeMap(specification) {
	}

	DirectXTextureCubeMap::~DirectXTextureCubeMap() = default;

	bool DirectXTextureCubeMap::LoadFromPath(const std::string&) { return false; }
	bool DirectXTextureCubeMap::LoadFromMemory(ByteView) { return false; }
	bool DirectXTextureCubeMap::LoadFromData(ByteView) { return false; }

	bool DirectXTextureCubeMap::LoadFaceFromPath(Face, const std::string&) { return false; }
	bool DirectXTextureCubeMap::LoadFaceFromMemory(Face, ByteView) { return false; }
	bool DirectXTextureCubeMap::LoadFaceFromData(Face, ByteView, uint32_t, uint32_t, uint32_t) { return false; }

	void DirectXTextureCubeMap::Bind(int) const {}
	void DirectXTextureCubeMap::Unbind() const {}
}
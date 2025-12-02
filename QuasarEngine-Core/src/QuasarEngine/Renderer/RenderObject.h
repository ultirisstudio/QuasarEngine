#pragma once

#include <glm/glm.hpp>

#include <QuasarEngine/Entity/Entity.h>

namespace QuasarEngine
{
	enum class RenderFlags : uint32_t {
		None = 0,
		Skinned = 1 << 0,
		Terrain = 1 << 1,
		PointCloud = 1 << 2,
		Instanced = 1 << 3,
	};

	inline RenderFlags operator|(RenderFlags a, RenderFlags b) {
		return RenderFlags(uint32_t(a) | uint32_t(b));
	}
	inline bool HasFlag(RenderFlags f, RenderFlags flag) {
		return (uint32_t(f) & uint32_t(flag)) != 0;
	}

	class Entity;
	class Mesh;
	class Material;

	struct RenderObject
	{
		glm::mat4 model;
		Mesh* mesh;
		Material* material;
		Entity entity;

		uint32_t instanceCount = 1;
		RenderFlags flags = RenderFlags::None;
	};
}
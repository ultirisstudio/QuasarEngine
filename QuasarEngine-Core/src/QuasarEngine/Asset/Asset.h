#pragma once

#include <QuasarEngine/Core/UUID.h>

namespace QuasarEngine
{
	enum AssetType : uint8_t
	{
		NONE = 0,
		TEXTURE,
		MODEL,
		QASSET,
		MESH
	};

	class Asset
	{
	public:
		UUID ID;

		virtual AssetType GetType() = 0;
	};
}
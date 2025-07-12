#pragma once

#include <string>

namespace QuasarEngine
{
	struct AssetHeader {
		uint32_t magic;
		uint32_t totalSize;
		std::string assetType;
	};
}
#pragma once

#include <vector>
#include <unordered_map>
#include <string>
#include <filesystem>

namespace QuasarEngine
{
	class Export
	{
	public:
		struct PakHeader {
			uint32_t magic;
			uint32_t totalSize;
			uint32_t numResources;
		};

		struct PakEntry {
			std::string name;
			uint32_t offset;
			uint32_t size;
			uint8_t flags;
		};

		static bool AddResourceToPak(const std::filesystem::path& resourcePath, std::ofstream& pakFile, bool isCompressed);
		static bool CreatePakFile(const std::vector<std::filesystem::path>& resources, const std::string& pakFilePath, bool isCompressed);

		static std::unordered_map<std::string, std::vector<char>> LoadAllResourcesFromPak(const std::string& pakFilePath);
	};
}
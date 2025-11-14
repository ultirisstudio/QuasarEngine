#pragma once

#include <fstream>
#include <string>
#include <sstream>
#include <vector>

namespace QuasarEngine::FileUtils
{
    inline std::vector<std::uint8_t> ReadFileBinary(const std::string& path) {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file) return {};
        const auto size = static_cast<std::size_t>(file.tellg());
        if (!size) return {};
        std::vector<std::uint8_t> buffer(size);
        file.seekg(0, std::ios::beg);
        file.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(size));
        return buffer;
    }

	inline std::unique_ptr<unsigned char[]> ReadFileToBuffer(const std::string& path, size_t& outSize)
	{
		outSize = 0;

		std::ifstream file(path, std::ios::binary | std::ios::ate);
		if (!file.is_open())
			return nullptr;

		std::streamsize fileSize = file.tellg();
		if (fileSize <= 0)
			return nullptr;

		outSize = static_cast<size_t>(fileSize);

		file.seekg(0, std::ios::beg);

		std::unique_ptr<unsigned char[]> buffer(new unsigned char[outSize]);

		if (!file.read(reinterpret_cast<char*>(buffer.get()), fileSize))
		{
			outSize = 0;
			return nullptr;
		}

		return buffer;
	}
}
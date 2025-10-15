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
}
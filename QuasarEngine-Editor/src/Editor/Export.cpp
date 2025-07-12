#include "Export.h"

#include <iostream>
#include <fstream>
#include <algorithm>

#include <intrin.h>

#include "Tools/Utils.h"

namespace QuasarEngine
{
    bool Export::AddResourceToPak(const std::filesystem::path& resourcePath, std::ofstream& pakFile, bool isCompressed = false)
    {
        std::ifstream resource(resourcePath, std::ios::binary);
        if (!resource.is_open()) {
            return false;
        }
        
        resource.seekg(0, std::ios::end);
        size_t fileSize = resource.tellg();
        std::vector<char> data(fileSize);
        resource.seekg(0, std::ios::beg);
        resource.read(data.data(), fileSize);
        
        if (isCompressed) {
            std::vector<char> compressedData = Utils::compress(data);
            size_t compressedSize = compressedData.size();
            data = compressedData;
            fileSize = compressedSize;
        }
        
        PakEntry entry;
        entry.name = resourcePath.filename().string();
        entry.offset = pakFile.tellp();
        entry.size = fileSize;
        entry.flags = isCompressed ? 1 : 0;
        pakFile.write((char*)&entry, sizeof(PakEntry));

        pakFile.write(data.data(), fileSize);
        
        return true;
    }

    bool Export::CreatePakFile(const std::vector<std::filesystem::path>& resources, const std::string& pakFilePath, bool isCompressed = false)
    {
        std::ofstream pakFile(pakFilePath, std::ios::binary);
        if (!pakFile.is_open()) {
            return false;
        }

        uint32_t totalSize = 0;

        PakHeader header;
        header.magic = 1;
        header.numResources = resources.size();
        pakFile.write((char*)&header, sizeof(PakHeader));

        for (const auto& resource : resources) {
            if (!AddResourceToPak(resource, pakFile, isCompressed)) {
                return false;
            }
        }

        totalSize = pakFile.tellp();

        //pakFile.seekp(offsetof(PakHeader, totalSize), std::ios::beg);
        pakFile.seekp((const int)(void*)&((PakHeader*)0)->totalSize, std::ios::beg);
        pakFile.write((char*)&totalSize, sizeof(uint32_t));

        pakFile.close();
        return true;
    }

    std::unordered_map<std::string, std::vector<char>> Export::LoadAllResourcesFromPak(const std::string& pakFilePath)
    {
        std::unordered_map<std::string, std::vector<char>> resources;

        std::ifstream pakFile(pakFilePath, std::ios::binary);
        if (!pakFile.is_open()) {
            return resources;
        }

        PakHeader header;
        pakFile.read((char*)&header, sizeof(PakHeader));
        if (!pakFile.good()) {
            return resources;
        }

        for (int i = 0; i < header.numResources; ++i) {
            PakEntry entry;
            pakFile.read((char*)&entry, sizeof(PakEntry));
            if (!pakFile.good()) {
                break;
            }

            std::vector<char> data;
            if (entry.flags & 1) {
                data.resize(entry.size);
                pakFile.read(data.data(), entry.size);
                if (!pakFile.good()) {
                    break;
                }

                std::vector<char> decompressedData = Utils::decompress(data);
                size_t decompressedSize = decompressedData.size();
                data = decompressedData;
                entry.size = decompressedSize;
            }
            else {
                data.resize(entry.size);
                pakFile.read(data.data(), entry.size);
                if (!pakFile.good()) {
                    break;
                }
            }

            resources[entry.name] = std::move(data);
        }

        pakFile.close();
        return resources;
    }
}
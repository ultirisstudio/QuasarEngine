#include "qepch.h"
#include "Utils.h"

#include <fstream>
#include <stdexcept>
#include <iomanip>
#include <sstream>
#include <filesystem>

#include <zlib.h>
#include <tinyfiledialogs/tinyfiledialogs.h>

#include <QuasarEngine/Core/Logger.h>

namespace QuasarEngine
{
	std::vector<char> Utils::readFile(const std::string& filePath)
    {
        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open())
        {
            Q_ERROR("Erreur lors de l'ouverture du fichier : " + filePath);
            return {};
        }

        file.seekg(0, std::ios::end);
        std::streampos size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<char> buffer(size);
        if (file.read(buffer.data(), size))
        {
            return buffer;
        }
        else {
            Q_ERROR("Erreur lors de la lecture du fichier : " + filePath);
            return {};
        }
    }

    std::string Utils::getFileExtension(const std::string& filename)
    {
        size_t pos = filename.find_last_of(".");
        if (pos == std::string::npos)
        {
            return "";
        }
        else
        {
            return filename.substr(pos + 1);
        }
    }

    std::optional<Utils::FileInfo> Utils::openFile()
    {
        char const* result = tinyfd_openFileDialog(
            "Open File",
            "",
            0,
            NULL,
            NULL,
            0
        );

        if (!result)
        {
            Q_ERROR("No file selected");
            return {};
        }

        Q_INFO(result);

        return getFileInfos(result);
    }

    std::optional<std::vector<Utils::FileInfo>> Utils::openFiles()
    {
        char const* result = tinyfd_openFileDialog(
            "Open File",
            "",
            0,
            NULL,
            NULL,
            1
        );

        if (!result)
        {
            Q_WARNING("No file selected");
            return std::nullopt;
        }

        std::vector<std::string> paths;
        std::stringstream ss(result);
        std::string path;

        while (std::getline(ss, path, '|')) {
            paths.push_back(path);
        }

        std::vector<Utils::FileInfo> fileInfoResult;
        fileInfoResult.reserve(paths.size());

        for (const auto& path : paths) {
			fileInfoResult.push_back(getFileInfos(path));
		}

		if (!fileInfoResult.empty()) {
			return fileInfoResult;
		}

        return std::nullopt;
    }

    std::optional<Utils::FileInfo> Utils::saveFile()
    {
        char const* lFilterPatterns[1] = { "*.scene" };

        char const* result = tinyfd_saveFileDialog(
            "Save File",
            "",
            1,
            lFilterPatterns,
            "Scene files"
        );

        if (!result)
        {
            Q_ERROR("No path selected");
            return {};
        }

        Q_INFO(result);

        return getFileInfos(result);
    }

    char* Utils::openFolder()
    {
        return tinyfd_selectFolderDialog("Open", "");
    }

    Utils::FileInfo Utils::getFileInfos(const std::string& filePath)
    {
        Utils::FileInfo infos;
        infos.filePath = filePath;

        const std::filesystem::path path(filePath);

        infos.selectedFile = path.filename().string();
        infos.fileExtension = path.extension().string();
        infos.fileName = path.stem().string();

        return infos;
    }

    std::vector<char> Utils::compress(const std::vector<char>& str)
    {
        z_stream zs;
        memset(&zs, 0, sizeof(zs));

        if (deflateInit(&zs, Z_BEST_COMPRESSION) != Z_OK) {
            throw(std::runtime_error("deflateInit failed while compressing."));
        }

        zs.next_in = (Bytef*)str.data();
        zs.avail_in = str.size();

        int ret;
        char outbuffer[10240];
        std::vector<char> compressedData;

        do {
            zs.next_out = reinterpret_cast<Bytef*>(outbuffer);
            zs.avail_out = sizeof(outbuffer);

            ret = deflate(&zs, Z_FINISH);

            if (compressedData.size() < zs.total_out) {
                compressedData.insert(compressedData.end(), outbuffer, outbuffer + zs.total_out - compressedData.size());
            }
        } while (ret == Z_OK);

        deflateEnd(&zs);

        if (ret != Z_STREAM_END) {
            std::ostringstream oss;
            oss << "Exception during zlib compression: (" << ret << ") " << zs.msg;
            throw(std::runtime_error(oss.str()));
        }

        return compressedData;
    }

    std::vector<char> Utils::decompress(const std::vector<char>& data) {
        z_stream zs;
        memset(&zs, 0, sizeof(zs));

        if (inflateInit(&zs) != Z_OK) {
            throw(std::runtime_error("inflateInit failed while decompressing."));
        }

        zs.next_in = (Bytef*)data.data();
        zs.avail_in = data.size();

        int ret;
        char outbuffer[10240];
        std::vector<char> decompressedData;

        do {
            zs.next_out = reinterpret_cast<Bytef*>(outbuffer);
            zs.avail_out = sizeof(outbuffer);

            ret = inflate(&zs, 0);

            if (decompressedData.size() < zs.total_out) {
                decompressedData.insert(decompressedData.end(), outbuffer, outbuffer + zs.total_out - decompressedData.size());
            }

        } while (ret == Z_OK);

        inflateEnd(&zs);

        if (ret != Z_STREAM_END) {
            std::ostringstream oss;
            oss << "Exception during zlib decompression: (" << ret << ") " << zs.msg;
            throw(std::runtime_error(oss.str()));
        }

        return decompressedData;
    }

    std::string Utils::toLower(std::string s)
    {
        for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return s;
    }

    bool Utils::Contains(const std::string& hay, const std::string& needle)
    {
        if (needle.empty()) return true;
        return toLower(hay).find(toLower(needle)) != std::string::npos;
    }
}
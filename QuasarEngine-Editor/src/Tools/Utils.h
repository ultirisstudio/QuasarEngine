#pragma once

#include <string>
#include <vector>
#include <optional>

namespace QuasarEngine
{
	class Utils
	{
	public:
		struct FileInfo
		{
			std::string selectedFile;
			std::string filePath;
			std::string fileExtension;
			std::string fileName;

			FileInfo() : selectedFile(""), filePath(""), fileExtension(""), fileName("") {}
		};

		static std::vector<char> readFile(const std::string& filePath);

		static std::string getFileExtension(const std::string& filePath);

		static std::optional<FileInfo> openFile();
		static std::optional<std::vector<FileInfo>> openFiles();
		static std::optional<FileInfo> saveFile();
		static char* openFolder();

		static FileInfo getFileInfos(const std::string& filePath);

		static std::vector<char> compress(const std::vector<char>& str);
		static std::vector<char> decompress(const std::vector<char>& str);

		static std::optional<std::string> getRelativePath(const std::string& sourcePath, const std::string& basePath) {
			if (sourcePath.find(basePath) != 0) {
				return sourcePath;
			}

			size_t commonLength = basePath.length();
			return sourcePath.substr(commonLength + 1);
		}
	};
}
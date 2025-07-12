#pragma once

#include <fstream>
#include <string>
#include <sstream>
#include <vector>

namespace QuasarEngine
{
    class FileUtils {
    public:
        static std::string readFile(const std::string& filePath) {
            std::ifstream file(filePath);
            std::stringstream buffer;
            buffer << file.rdbuf();
            return buffer.str();
        }

        static std::vector<unsigned char> readBinaryFile(const std::string& filename) {
            std::ifstream file(filename, std::ios::ate | std::ios::binary);

            if (!file.is_open()) {
                throw std::runtime_error("failed to open file!");
            }

            size_t fileSize = (size_t)file.tellg();
            std::vector<unsigned char> buffer(fileSize);

            file.seekg(0);
            file.read(reinterpret_cast<char*>(buffer.data()), fileSize);

            file.close();

            return buffer;
        }

        static bool writeFile(const std::string& filePath, const std::string& content) {
            std::ofstream file(filePath);
            if (file.is_open()) {
                file << content;
                return true;
            }
            return false;
        }

        static std::vector<std::string> readLines(const std::string& filePath) {
            std::ifstream file(filePath);
            std::vector<std::string> lines;
            std::string line;
            while (std::getline(file, line)) {
                lines.push_back(line);
            }
            return lines;
        }
    };
}
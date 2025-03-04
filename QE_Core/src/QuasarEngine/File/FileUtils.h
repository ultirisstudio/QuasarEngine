#pragma once

#include <fstream>
#include <string>
#include <sstream>
#include <vector>

class FileUtils {
public:
    static std::string readFile(const std::string& filePath) {
        std::ifstream file(filePath);
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
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

#pragma once

#include "JsonValue.h"
#include "JsonObject.h"
#include "JsonArray.h"
#include "JsonPrimitive.h"

#include <fstream>
#include <iostream>

namespace QuasarEngine
{
    class JsonWriter {
    public:
        explicit JsonWriter(const std::string& filename) : outFile(filename) {
            if (!outFile.is_open()) {
                throw std::runtime_error("Unable to open file for writing.");
            }
        }

        void write(const std::shared_ptr<JsonValue>& root) {
            outFile << root->toString() << std::endl;
        }

    private:
        std::ofstream outFile;
    };
}
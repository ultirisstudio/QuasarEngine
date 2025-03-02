#pragma once

#include <string>

class DynamicLoader {
public:
    static void* LoadRendererDLL(const std::string& path);
    static void* LoadGUIDLL(const std::string& path);
};

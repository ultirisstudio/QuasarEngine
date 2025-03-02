#include "DynamicLoader.h"
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

void* DynamicLoader::LoadRendererDLL(const std::string& path) {
#ifdef _WIN32
    HMODULE handle = LoadLibraryA(path.c_str());
    if (!handle) {
        std::cerr << "Failed to load DLL: " << path << std::endl;
    }
    return (void*)handle;
#else
    void* handle = dlopen(path.c_str(), RTLD_LAZY);
    if (!handle) {
        std::cerr << "Failed to load shared library: " << path << std::endl;
    }
    return handle;
#endif
}

void* DynamicLoader::LoadGUIDLL(const std::string& path) {
    return LoadRendererDLL(path);
}

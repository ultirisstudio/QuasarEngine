#include "EngineFactory.h"

#include <stdexcept>

EngineFactory& EngineFactory::Instance() {
    static EngineFactory instance;
    return instance;
}

void EngineFactory::RegisterRenderer(RendererAPI api, RendererCreator creator) {
    rendererCreators[api] = std::move(creator);
}

void EngineFactory::RegisterGUI(GUIAPI api, GUICreator creator) {
    guiCreators[api] = std::move(creator);
}

void EngineFactory::RegisterWindow(WindowAPI api, WindowCreator creator) {
    windowCreators[api] = std::move(creator);
}

std::unique_ptr<IRenderer> EngineFactory::CreateRenderer(RendererAPI api) {
    auto it = rendererCreators.find(api);
    if (it != rendererCreators.end()) {
        return it->second();
    }
    throw std::runtime_error("Renderer API non enregistrée !");
}

std::unique_ptr<IGUI> EngineFactory::CreateGUI(GUIAPI api) {
    auto it = guiCreators.find(api);
    if (it != guiCreators.end()) {
        return it->second();
    }
    throw std::runtime_error("GUI API non enregistrée !");
}

std::unique_ptr<IWindow> EngineFactory::CreateWindow(WindowAPI api) {
    auto it = windowCreators.find(api);
    if (it != windowCreators.end()) {
        return it->second();
    }
    throw std::runtime_error("Window API non enregistrée !");
}
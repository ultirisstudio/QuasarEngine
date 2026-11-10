#pragma once

#include <quasar/core/Base.h>
#include <quasar/renderer/GraphicsAPI.h>
#include <quasar/renderer/GraphicsContext.h>

#include <SDL3/SDL_video.h>

namespace Quasar
{
    SDL_WindowFlags PrepareGraphicsWindow(GraphicsAPI api);

    Scope<GraphicsContext> CreateGraphicsContext(GraphicsAPI api, SDL_Window* window);
}

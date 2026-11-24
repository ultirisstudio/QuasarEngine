#include <io/SdlFileDialogBackend.h>

#include <quasar/core/Window.h>
#include <quasar/io/Path.h>

#include <SDL3/SDL_dialog.h>
#include <SDL3/SDL_error.h>

#include <memory>
#include <string>
#include <utility>

namespace Quasar
{
    namespace
    {
        struct Request
        {
            FileDialogBackend::Callback Done;
            std::string DefaultLocation;
        };

        void OnFolderChosen(void* userdata, const char* const* fileList, [[maybe_unused]] int filter)
        {
            const std::unique_ptr<Request> request(static_cast<Request*>(userdata));
            FileDialogResult result;
            if (fileList == nullptr)
            {
                result.Error = SDL_GetError();
            }
            else if (const char* folder = *fileList; folder != nullptr && *folder != '\0')
            {
                result.Folder = PathFromUtf8(folder);
            }
            request->Done(std::move(result));
        }
    }

    void SdlFileDialogBackend::ShowFolderDialog(const Window* window, const std::filesystem::path& defaultLocation, Callback callback)
    {
        auto request = std::make_unique<Request>(Request{.Done = std::move(callback), .DefaultLocation = PathToUtf8(defaultLocation)});
        const char* location = request->DefaultLocation.empty() ? nullptr : request->DefaultLocation.c_str();
        SDL_ShowOpenFolderDialog(OnFolderChosen, request.release(), window != nullptr ? window->GetNativeWindow() : nullptr, location, false);
    }
}

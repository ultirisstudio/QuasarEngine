#include "FolderDialog.h"

#include "Project.h"

#include <quasar/io/Path.h>

#include <SDL3/SDL_dialog.h>
#include <SDL3/SDL_error.h>

#include <mutex>
#include <utility>

namespace Quasar::Editor
{
    struct FolderDialogState
    {
        std::mutex Mutex;
        bool Open = false;
        std::string DefaultLocation;
        std::optional<std::filesystem::path> Folder;
        std::string Error;
    };

    namespace
    {
        using SharedState = std::shared_ptr<FolderDialogState>;

        void SDLCALL OnFolderChosen(void* userdata, const char* const* fileList, [[maybe_unused]] int filter)
        {
            const std::unique_ptr<SharedState> owner(static_cast<SharedState*>(userdata));
            FolderDialogState& state = **owner;

            const std::scoped_lock lock(state.Mutex);
            state.Open = false;
            if (fileList == nullptr)
            {
                state.Error = std::string("Cannot show the folder dialog: ") + SDL_GetError();
            }
            else if (const char* folder = *fileList; folder != nullptr)
            {
                state.Folder = PathFromUtf8(folder);
            }
        }
    }

    FolderDialog::FolderDialog() : m_State(std::make_shared<FolderDialogState>()) {}

    void FolderDialog::Open(SDL_Window* window, const std::filesystem::path& defaultLocation)
    {
        {
            const std::scoped_lock lock(m_State->Mutex);
            if (m_State->Open)
            {
                return;
            }
            m_State->Open = true;
            m_State->Folder.reset();
            m_State->Error.clear();
            m_State->DefaultLocation = PathToUtf8(defaultLocation);
        }

        auto owner = std::make_unique<SharedState>(m_State);
        const char* location = m_State->DefaultLocation.empty() ? nullptr : m_State->DefaultLocation.c_str();
        SDL_ShowOpenFolderDialog(OnFolderChosen, owner.release(), window, location, false);
    }

    bool FolderDialog::IsOpen() const
    {
        const std::scoped_lock lock(m_State->Mutex);
        return m_State->Open;
    }

    std::optional<std::filesystem::path> FolderDialog::TakeFolder()
    {
        const std::scoped_lock lock(m_State->Mutex);
        return std::exchange(m_State->Folder, std::nullopt);
    }

    std::string FolderDialog::TakeError()
    {
        const std::scoped_lock lock(m_State->Mutex);
        return std::exchange(m_State->Error, std::string());
    }
}

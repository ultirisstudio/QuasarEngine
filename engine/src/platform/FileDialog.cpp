#include <quasar/platform/FileDialog.h>

#include <quasar/core/Window.h>
#include <quasar/io/Path.h>
#include <quasar/log/Log.h>

#include <SDL3/SDL_dialog.h>
#include <SDL3/SDL_error.h>

#include <mutex>
#include <string>
#include <utility>

namespace Quasar
{
    struct FileDialog::State
    {
        std::mutex Mutex;
        bool Open = false;
        std::string DefaultLocation;
        std::optional<std::filesystem::path> Folder;
    };

    FileDialog::FileDialog() : m_State(std::make_shared<State>()) {}

    void FileDialog::OpenFolder(const Window* window, const std::filesystem::path& defaultLocation)
    {
        {
            const std::scoped_lock lock(m_State->Mutex);
            if (m_State->Open)
            {
                return;
            }
            m_State->Open = true;
            m_State->Folder.reset();
            m_State->DefaultLocation = PathToUtf8(defaultLocation);
        }

        auto owner = std::make_unique<std::shared_ptr<State>>(m_State);
        const char* location = m_State->DefaultLocation.empty() ? nullptr : m_State->DefaultLocation.c_str();
        SDL_ShowOpenFolderDialog(OnFolderChosen, owner.release(), window != nullptr ? window->GetNativeWindow() : nullptr, location, false);
    }

    bool FileDialog::IsOpen() const
    {
        const std::scoped_lock lock(m_State->Mutex);
        return m_State->Open;
    }

    std::optional<std::filesystem::path> FileDialog::TakeFolder()
    {
        const std::scoped_lock lock(m_State->Mutex);
        return std::exchange(m_State->Folder, std::nullopt);
    }

    void FileDialog::OnFolderChosen(void* userdata, const char* const* fileList, [[maybe_unused]] int filter)
    {
        const std::unique_ptr<std::shared_ptr<State>> owner(static_cast<std::shared_ptr<State>*>(userdata));
        State& state = **owner;

        const std::scoped_lock lock(state.Mutex);
        state.Open = false;
        if (fileList == nullptr)
        {
            QUASAR_ENGINE_ERROR("Cannot show the folder dialog: {}", SDL_GetError());
        }
        else if (const char* folder = *fileList; folder != nullptr && *folder != '\0')
        {
            state.Folder = PathFromUtf8(folder);
        }
    }
}

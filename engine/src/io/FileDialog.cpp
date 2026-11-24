#include <quasar/io/FileDialog.h>

#include <io/SdlFileDialogBackend.h>

#include <quasar/log/Log.h>

#include <mutex>
#include <utility>

namespace Quasar
{
    struct FileDialog::State
    {
        std::mutex Mutex;
        bool Open = false;
        std::optional<std::filesystem::path> Folder;
    };

    FileDialog::FileDialog(Scope<FileDialogBackend> backend)
        : m_State(std::make_shared<State>()), m_Backend(backend != nullptr ? std::move(backend) : CreateScope<SdlFileDialogBackend>())
    {
    }

    FileDialog::~FileDialog() = default;

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
        }

        m_Backend->ShowFolderDialog(window, defaultLocation,
                                    [state = m_State](FileDialogResult result)
                                    {
                                        const std::scoped_lock lock(state->Mutex);
                                        state->Open = false;
                                        if (!result.Error.empty())
                                        {
                                            QUASAR_ENGINE_ERROR("Cannot show the folder dialog: {}", result.Error);
                                        }
                                        state->Folder = std::move(result.Folder);
                                    });
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
}

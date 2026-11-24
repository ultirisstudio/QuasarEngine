#pragma once

#include <quasar/core/Base.h>
#include <quasar/io/FileDialog.h>

#include <filesystem>
#include <utility>

namespace TestHelpers
{
    struct FolderDialogRequest
    {
        const Quasar::Window* Window = nullptr;
        std::filesystem::path DefaultLocation;
        Quasar::FileDialogBackend::Callback Callback;
        int Count = 0;
    };

    class FakeFileDialogBackend final : public Quasar::FileDialogBackend
    {
    public:
        explicit FakeFileDialogBackend(FolderDialogRequest& request) : m_Request(&request) {}

        void ShowFolderDialog(const Quasar::Window* window, const std::filesystem::path& defaultLocation, Callback callback) override
        {
            m_Request->Window = window;
            m_Request->DefaultLocation = defaultLocation;
            m_Request->Callback = std::move(callback);
            ++m_Request->Count;
        }

    private:
        FolderDialogRequest* m_Request;
    };

    inline Quasar::Scope<Quasar::FileDialogBackend> CreateFakeFileDialogBackend(FolderDialogRequest& request)
    {
        return Quasar::CreateScope<FakeFileDialogBackend>(request);
    }
}

#pragma once

#include <quasar/io/FileDialog.h>

namespace Quasar
{
    class SdlFileDialogBackend final : public FileDialogBackend
    {
    public:
        void ShowFolderDialog(const Window* window, const std::filesystem::path& defaultLocation, Callback callback) override;
    };
}

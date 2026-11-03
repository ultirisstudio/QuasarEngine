#pragma once

#include <quasar/core/UUID.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <system_error>

namespace TestHelpers
{
    class TemporaryFolder
    {
    public:
        explicit TemporaryFolder(std::string_view name = "quasar_test")
            : m_Path(std::filesystem::temp_directory_path() / (std::string(name) + "_" + Quasar::UUID::Generate().ToString()))
        {
            std::error_code error;
            std::filesystem::create_directories(m_Path, error);
        }

        ~TemporaryFolder()
        {
            std::error_code error;
            std::filesystem::remove_all(m_Path, error);
        }

        TemporaryFolder(const TemporaryFolder&) = delete;
        TemporaryFolder& operator=(const TemporaryFolder&) = delete;
        TemporaryFolder(TemporaryFolder&&) = delete;
        TemporaryFolder& operator=(TemporaryFolder&&) = delete;

        const std::filesystem::path& GetPath() const { return m_Path; }

    private:
        std::filesystem::path m_Path;
    };

    class TemporaryFile
    {
    public:
        TemporaryFile(std::string_view name, std::string_view content) : m_Path(m_Folder.GetPath() / name)
        {
            std::ofstream stream(m_Path, std::ios::binary);
            stream.write(content.data(), static_cast<std::streamsize>(content.size()));
        }

        const std::filesystem::path& GetPath() const { return m_Path; }

    private:
        TemporaryFolder m_Folder;
        std::filesystem::path m_Path;
    };

    class ScopedCurrentDirectory
    {
    public:
        ScopedCurrentDirectory() : m_Path(std::filesystem::current_path()) {}

        ~ScopedCurrentDirectory()
        {
            std::error_code error;
            std::filesystem::current_path(m_Path, error);
        }

        ScopedCurrentDirectory(const ScopedCurrentDirectory&) = delete;
        ScopedCurrentDirectory& operator=(const ScopedCurrentDirectory&) = delete;
        ScopedCurrentDirectory(ScopedCurrentDirectory&&) = delete;
        ScopedCurrentDirectory& operator=(ScopedCurrentDirectory&&) = delete;

        const std::filesystem::path& GetPath() const { return m_Path; }

    private:
        std::filesystem::path m_Path;
    };
}
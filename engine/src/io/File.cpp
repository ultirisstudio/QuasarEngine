#include <quasar/io/File.h>
#include <quasar/log/Log.h>

#include <spdlog/fmt/std.h>

#include <fstream>
#include <span>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>

namespace Quasar
{
    namespace
    {
        template <typename Container>
        std::optional<Container> ReadWholeFile(const std::filesystem::path& path, std::string& reason)
        {
            std::error_code error;
            const std::filesystem::file_status status = std::filesystem::status(path, error);
            if (!std::filesystem::exists(status))
            {
                reason = fmt::format("Cannot read '{}': the file does not exist", path);
                return std::nullopt;
            }
            if (!std::filesystem::is_regular_file(status))
            {
                reason = fmt::format("Cannot read '{}': this is not a regular file", path);
                return std::nullopt;
            }

            const std::size_t size = std::filesystem::file_size(path, error);
            if (error)
            {
                reason = fmt::format("Cannot read '{}': {}", path, error.message());
                return std::nullopt;
            }

            std::ifstream stream(path, std::ios::binary);
            if (!stream)
            {
                reason = fmt::format("Cannot open '{}'", path);
                return std::nullopt;
            }

            Container content(size, typename Container::value_type{});
            if (size == 0)
            {
                return content;
            }

            const auto byteCount = static_cast<std::streamsize>(size);
            if constexpr (std::is_same_v<typename Container::value_type, char>)
            {
                stream.read(content.data(), byteCount);
            }
            else
            {
                // Streams only read characters, and any object can be read as characters
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
                stream.read(reinterpret_cast<char*>(content.data()), byteCount);
            }

            if (stream.gcount() != byteCount)
            {
                reason = fmt::format("Cannot read '{}': expected {} bytes, got {}", path, size, stream.gcount());
                return std::nullopt;
            }

            return content;
        }

        std::string WriteWholeFile(const std::filesystem::path& path, const char* data, std::size_t size)
        {
            if (path.has_parent_path())
            {
                std::error_code error;
                std::filesystem::create_directories(path.parent_path(), error);
                if (error)
                {
                    return fmt::format("Cannot write '{}': its directory cannot be created: {}", path, error.message());
                }
            }

            std::ofstream stream(path, std::ios::binary | std::ios::trunc);
            if (!stream)
            {
                return fmt::format("Cannot open '{}' for writing", path);
            }

            stream.write(data, static_cast<std::streamsize>(size));
            stream.close();
            if (!stream)
            {
                return fmt::format("Cannot write '{}'", path);
            }

            return {};
        }

        template <typename Container>
        std::optional<Container> ReadAndLog(const std::filesystem::path& path)
        {
            std::string error;
            std::optional<Container> content = ReadWholeFile<Container>(path, error);
            if (!content)
            {
                QUASAR_ENGINE_ERROR("{}", error);
            }
            return content;
        }

        std::optional<std::string> RemoveByteOrderMark(std::optional<std::string> text)
        {
            constexpr std::string_view byteOrderMark = "\xEF\xBB\xBF";
            if (text && text->starts_with(byteOrderMark))
            {
                text->erase(0, byteOrderMark.size());
            }
            return text;
        }

        bool LogWriteError(const std::string& error)
        {
            if (!error.empty())
            {
                QUASAR_ENGINE_ERROR("{}", error);
            }
            return error.empty();
        }
    }

    std::optional<std::string> ReadTextFile(const std::filesystem::path& path)
    {
        return RemoveByteOrderMark(ReadAndLog<std::string>(path));
    }

    std::optional<std::string> ReadTextFile(const std::filesystem::path& path, std::string& error)
    {
        return RemoveByteOrderMark(ReadWholeFile<std::string>(path, error));
    }

    std::optional<std::vector<std::byte>> ReadBinaryFile(const std::filesystem::path& path)
    {
        return ReadAndLog<std::vector<std::byte>>(path);
    }

    bool WriteTextFile(const std::filesystem::path& path, std::string_view text)
    {
        return LogWriteError(WriteWholeFile(path, text.data(), text.size()));
    }

    bool WriteTextFile(const std::filesystem::path& path, std::string_view text, std::string& error)
    {
        std::string reason = WriteWholeFile(path, text.data(), text.size());
        if (!reason.empty())
        {
            error = std::move(reason);
            return false;
        }
        return true;
    }

    bool WriteBinaryFile(const std::filesystem::path& path, std::span<const std::byte> data)
    {
        // Streams only write characters, and any object can be read as characters
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        return LogWriteError(WriteWholeFile(path, reinterpret_cast<const char*>(data.data()), data.size()));
    }

    bool CreateDirectories(const std::filesystem::path& path)
    {
        std::error_code error;
        std::filesystem::create_directories(path, error);
        if (error)
        {
            QUASAR_ENGINE_ERROR("Cannot create the directory '{}': {}", path, error.message());
            return false;
        }

        if (!std::filesystem::is_directory(path, error))
        {
            QUASAR_ENGINE_ERROR("Cannot create the directory '{}': a file has this name", path);
            return false;
        }

        return true;
    }
}

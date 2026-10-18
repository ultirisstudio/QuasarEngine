#include <quasar/core/Log.h>
#include <quasar/io/File.h>

#include <spdlog/fmt/std.h>

#include <fstream>
#include <string_view>
#include <system_error>
#include <type_traits>

namespace Quasar
{
    namespace
    {
        template <typename Container>
        std::optional<Container> ReadWholeFile(const std::filesystem::path& path)
        {
            std::error_code error;
            const std::filesystem::file_status status = std::filesystem::status(path, error);
            if (!std::filesystem::exists(status))
            {
                QUASAR_ENGINE_ERROR("Cannot read '{}': the file does not exist", path);
                return std::nullopt;
            }
            if (!std::filesystem::is_regular_file(status))
            {
                QUASAR_ENGINE_ERROR("Cannot read '{}': this is not a regular file", path);
                return std::nullopt;
            }

            const std::size_t size = std::filesystem::file_size(path, error);
            if (error)
            {
                QUASAR_ENGINE_ERROR("Cannot read '{}': {}", path, error.message());
                return std::nullopt;
            }

            std::ifstream stream(path, std::ios::binary);
            if (!stream)
            {
                QUASAR_ENGINE_ERROR("Cannot open '{}'", path);
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
                stream.read(reinterpret_cast<char*>(content.data()), byteCount);
            }

            if (stream.gcount() != byteCount)
            {
                QUASAR_ENGINE_ERROR("Cannot read '{}': expected {} bytes, got {}", path, size, stream.gcount());
                return std::nullopt;
            }

            return content;
        }
    }

    std::optional<std::string> ReadTextFile(const std::filesystem::path& path)
    {
        std::optional<std::string> text = ReadWholeFile<std::string>(path);

        constexpr std::string_view byteOrderMark = "\xEF\xBB\xBF";
        if (text && text->starts_with(byteOrderMark))
        {
            text->erase(0, byteOrderMark.size());
        }

        return text;
    }

    std::optional<std::vector<std::byte>> ReadBinaryFile(const std::filesystem::path& path)
    {
        return ReadWholeFile<std::vector<std::byte>>(path);
    }
}

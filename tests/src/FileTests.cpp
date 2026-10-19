#include <quasar/core/Log.h>
#include <quasar/io/File.h>

#include <doctest/doctest.h>

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

using namespace std::string_view_literals;

namespace
{
    class TemporaryFile
    {
    public:
        TemporaryFile(std::string_view name, std::string_view content)
            : m_Path(std::filesystem::temp_directory_path() / name)
        {
            std::ofstream stream(m_Path, std::ios::binary);
            stream.write(content.data(), static_cast<std::streamsize>(content.size()));
        }

        ~TemporaryFile()
        {
            std::error_code error;
            std::filesystem::remove(m_Path, error);
        }

        TemporaryFile(const TemporaryFile&) = delete;
        TemporaryFile& operator=(const TemporaryFile&) = delete;

        const std::filesystem::path& GetPath() const { return m_Path; }

    private:
        std::filesystem::path m_Path;
    };
}

TEST_CASE("ReadTextFile returns the content exactly as stored")
{
    const TemporaryFile file("quasar_text_content.txt", "line 1\r\nline 2\n");

    const std::optional<std::string> text = Quasar::ReadTextFile(file.GetPath());

    REQUIRE(text.has_value());
    CHECK(*text == "line 1\r\nline 2\n");
}

TEST_CASE("ReadTextFile removes the UTF-8 byte order mark")
{
    const TemporaryFile file("quasar_text_bom.txt", "\xEF\xBB\xBFhello");

    const std::optional<std::string> text = Quasar::ReadTextFile(file.GetPath());

    REQUIRE(text.has_value());
    CHECK(*text == "hello");
}

TEST_CASE("ReadBinaryFile keeps every byte including zero bytes")
{
    const TemporaryFile file("quasar_binary_content.bin", "\x00\x01\xFF\x00"sv);

    const std::optional<std::vector<std::byte>> data = Quasar::ReadBinaryFile(file.GetPath());

    const std::vector<std::byte> expected{std::byte{0x00}, std::byte{0x01}, std::byte{0xFF}, std::byte{0x00}};
    REQUIRE(data.has_value());
    CHECK(*data == expected);
}

TEST_CASE("An empty file is read as empty content")
{
    const TemporaryFile file("quasar_empty.txt", "");

    const std::optional<std::string> text = Quasar::ReadTextFile(file.GetPath());
    const std::optional<std::vector<std::byte>> data = Quasar::ReadBinaryFile(file.GetPath());

    REQUIRE(text.has_value());
    CHECK(text->empty());
    REQUIRE(data.has_value());
    CHECK(data->empty());
}

TEST_CASE("A missing file gives no value")
{
    Quasar::Log::Init();

    const std::filesystem::path path = std::filesystem::temp_directory_path() / "quasar_missing_file.txt";

    CHECK_FALSE(Quasar::ReadTextFile(path).has_value());
    CHECK_FALSE(Quasar::ReadBinaryFile(path).has_value());

    Quasar::Log::Shutdown();
}

TEST_CASE("A folder is not read as a file")
{
    Quasar::Log::Init();

    const std::filesystem::path folder = std::filesystem::temp_directory_path();

    CHECK_FALSE(Quasar::ReadTextFile(folder).has_value());
    CHECK_FALSE(Quasar::ReadBinaryFile(folder).has_value());

    Quasar::Log::Shutdown();
}

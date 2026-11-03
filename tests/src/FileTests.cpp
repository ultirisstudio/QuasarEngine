#include "LogHelpers.h"
#include "TemporaryFiles.h"

#include <quasar/io/File.h>

#include <doctest/doctest.h>

#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

using namespace std::string_view_literals;

TEST_CASE("ReadTextFile returns the content exactly as stored")
{
    const TestHelpers::TemporaryFile file("quasar_text_content.txt", "line 1\r\nline 2\n");

    const std::optional<std::string> text = Quasar::ReadTextFile(file.GetPath());

    REQUIRE(text.has_value());
    CHECK(*text == "line 1\r\nline 2\n");
}

TEST_CASE("ReadTextFile removes the UTF-8 byte order mark")
{
    const TestHelpers::TemporaryFile file("quasar_text_bom.txt", "\xEF\xBB\xBFhello");

    const std::optional<std::string> text = Quasar::ReadTextFile(file.GetPath());

    REQUIRE(text.has_value());
    CHECK(*text == "hello");
}

TEST_CASE("ReadBinaryFile keeps every byte including zero bytes")
{
    const TestHelpers::TemporaryFile file("quasar_binary_content.bin", "\x00\x01\xFF\x00"sv);

    const std::optional<std::vector<std::byte>> data = Quasar::ReadBinaryFile(file.GetPath());

    const std::vector<std::byte> expected{std::byte{0x00}, std::byte{0x01}, std::byte{0xFF}, std::byte{0x00}};
    REQUIRE(data.has_value());
    CHECK(*data == expected);
}

TEST_CASE("An empty file is read as empty content")
{
    const TestHelpers::TemporaryFile file("quasar_empty.txt", "");

    const std::optional<std::string> text = Quasar::ReadTextFile(file.GetPath());
    const std::optional<std::vector<std::byte>> data = Quasar::ReadBinaryFile(file.GetPath());

    REQUIRE(text.has_value());
    CHECK(text->empty());
    REQUIRE(data.has_value());
    CHECK(data->empty());
}

TEST_CASE("A missing file gives no value")
{
    const TestHelpers::ScopedLog log;

    const TestHelpers::TemporaryFolder folder;
    const std::filesystem::path path = folder.GetPath() / "missing.txt";

    CHECK_FALSE(Quasar::ReadTextFile(path).has_value());
    CHECK_FALSE(Quasar::ReadBinaryFile(path).has_value());
}

TEST_CASE("A folder is not read as a file")
{
    const TestHelpers::ScopedLog log;

    const TestHelpers::TemporaryFolder folder;

    CHECK_FALSE(Quasar::ReadTextFile(folder.GetPath()).has_value());
    CHECK_FALSE(Quasar::ReadBinaryFile(folder.GetPath()).has_value());
}

TEST_CASE("Writing a file creates its missing parent directories")
{
    const TestHelpers::ScopedLog log;

    const TestHelpers::TemporaryFolder folder("quasar_write_parents");
    const std::filesystem::path path = folder.GetPath() / "Assets" / "Scenes" / "main.scene";

    REQUIRE(Quasar::WriteTextFile(path, "line 1\r\nline 2\n"));

    const std::optional<std::string> text = Quasar::ReadTextFile(path);
    REQUIRE(text.has_value());
    CHECK(*text == "line 1\r\nline 2\n");
}

TEST_CASE("WriteBinaryFile writes every byte including zero bytes")
{
    const TestHelpers::ScopedLog log;

    const TestHelpers::TemporaryFolder folder("quasar_write_binary");
    const std::filesystem::path path = folder.GetPath() / "data.bin";
    const std::vector<std::byte> data{std::byte{0x00}, std::byte{0x01}, std::byte{0xFF}, std::byte{0x00}};

    REQUIRE(Quasar::WriteBinaryFile(path, data));

    const std::optional<std::vector<std::byte>> read = Quasar::ReadBinaryFile(path);
    REQUIRE(read.has_value());
    CHECK(*read == data);
}

TEST_CASE("Writing a file replaces its previous content")
{
    const TestHelpers::ScopedLog log;

    const TestHelpers::TemporaryFolder folder("quasar_write_replace");
    const std::filesystem::path path = folder.GetPath() / "file.txt";

    REQUIRE(Quasar::WriteTextFile(path, "a long first content"));
    REQUIRE(Quasar::WriteTextFile(path, "short"));

    const std::optional<std::string> text = Quasar::ReadTextFile(path);
    REQUIRE(text.has_value());
    CHECK(*text == "short");
}

TEST_CASE("A file cannot be written inside a file")
{
    const TestHelpers::ScopedLog log;

    const TestHelpers::TemporaryFolder folder("quasar_write_inside_file");
    REQUIRE(Quasar::WriteTextFile(folder.GetPath() / "file.txt", "content"));

    CHECK_FALSE(Quasar::WriteTextFile(folder.GetPath() / "file.txt" / "inner.txt", "content"));
}

TEST_CASE("Creating a directory that already exists succeeds")
{
    const TestHelpers::ScopedLog log;

    const TestHelpers::TemporaryFolder folder("quasar_create_directories");
    const std::filesystem::path path = folder.GetPath() / "a" / "b";

    CHECK(Quasar::CreateDirectories(path));
    CHECK(Quasar::CreateDirectories(path));
    CHECK(std::filesystem::is_directory(path));
}

TEST_CASE("A directory cannot be created where a file exists")
{
    const TestHelpers::ScopedLog log;

    const TestHelpers::TemporaryFolder folder("quasar_create_over_file");
    const std::filesystem::path path = folder.GetPath() / "name";
    REQUIRE(Quasar::WriteTextFile(path, "content"));

    CHECK_FALSE(Quasar::CreateDirectories(path));
}
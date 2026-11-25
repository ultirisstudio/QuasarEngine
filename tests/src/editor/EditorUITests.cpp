#include <EditorUI.h>

#include <doctest/doctest.h>

#include <string>
#include <string_view>
#include <vector>

using Quasar::Editor::UI::WrapRightToLeft;

namespace
{
    float CountCharacters(std::string_view text)
    {
        return static_cast<float>(text.size());
    }
}

TEST_SUITE("EditorUI")
{
    TEST_CASE("A text written from right to left is wrapped from its end, which is read first")
    {
        CHECK(WrapRightToLeft("aa bb cc", 5.0f, CountCharacters) == std::vector<std::string>{"bb cc", "aa"});
        CHECK(WrapRightToLeft("aa bb cc", 8.0f, CountCharacters) == std::vector<std::string>{"aa bb cc"});
        CHECK(WrapRightToLeft("aa bb cc", 1.0f, CountCharacters) == std::vector<std::string>{"cc", "bb", "aa"});
    }

    TEST_CASE("Each line of a text written from right to left is wrapped on its own, and a long word stays whole")
    {
        CHECK(WrapRightToLeft("aa\nbb cc", 100.0f, CountCharacters) == std::vector<std::string>{"aa", "bb cc"});
        CHECK(WrapRightToLeft("abcdefgh", 3.0f, CountCharacters) == std::vector<std::string>{"abcdefgh"});
        CHECK(WrapRightToLeft("", 3.0f, CountCharacters) == std::vector<std::string>{""});
        CHECK(WrapRightToLeft("aa\n", 3.0f, CountCharacters) == std::vector<std::string>{"aa", ""});
    }
}

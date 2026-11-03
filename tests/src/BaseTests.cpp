#include <quasar/core/Base.h>

#include <doctest/doctest.h>

#include <string>
#include <utility>

namespace
{
    struct Foo
    {
        Foo(int value, std::string name) : Value(value), Name(std::move(name)) {}

        int Value;
        std::string Name;
    };

    QUASAR_FORCE_INLINE int Twice(int value)
    {
        return value * 2;
    }
}

TEST_CASE("CreateScope builds the object from the given arguments")
{
    Quasar::Scope<Foo> foo = Quasar::CreateScope<Foo>(42, "foo");
    REQUIRE(foo);
    CHECK(foo->Value == 42);
    CHECK(foo->Name == "foo");
}

TEST_CASE("CreateRef builds the object from the given arguments")
{
    const Quasar::Ref<Foo> foo = Quasar::CreateRef<Foo>(7, "bar");
    REQUIRE(foo.use_count() == 1);
    CHECK(foo->Value == 7);
    CHECK(foo->Name == "bar");
}

TEST_CASE("Bit sets only the requested bit")
{
    static_assert(Quasar::Bit(0) == 1u);
    static_assert(Quasar::Bit(1) == 2u);
    static_assert(Quasar::Bit(5) == 32u);
    static_assert(Quasar::Bit(31) == 0x80000000u);

    const std::uint32_t index = 4;
    CHECK(Quasar::Bit(index) == 16u);
}

TEST_CASE("A QUASAR_FORCE_INLINE function can be called")
{
    CHECK(Twice(21) == 42);
}

TEST_CASE("QUASAR_CONCAT joins two tokens into one name")
{
    const int QUASAR_CONCAT(value, 42) = 7;

    CHECK(value42 == 7);
}

TEST_CASE("QUASAR_CONCAT expands a macro before joining")
{
#define QUASAR_TEST_SUFFIX 5
    const int QUASAR_CONCAT(value, QUASAR_TEST_SUFFIX) = 3;
#undef QUASAR_TEST_SUFFIX

    CHECK(value5 == 3);
}

TEST_CASE("A passing assertion evaluates its condition only in Debug")
{
    int evaluations = 0;
    auto condition = [&evaluations]
    {
        ++evaluations;
        return true;
    };

    QUASAR_ENGINE_ASSERT(condition());
    QUASAR_ENGINE_ASSERT(condition(), "Message with an argument: {}", 42);

    CHECK(evaluations == (Quasar::PlatformDetection::IsDebug() ? 2 : 0));
}

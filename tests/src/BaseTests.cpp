#include <quasar/core/Base.h>

#include <doctest/doctest.h>

#include <string>
#include <utility>

namespace
{
    struct Foo
    {
        int Value;
        std::string Name;

        Foo(int value, std::string name) : Value(value), Name(std::move(name)) {}
    };
}

TEST_CASE("Scope")
{
    Quasar::Scope<Foo> foo = Quasar::CreateScope<Foo>(42, "foo");
    REQUIRE(foo);
    CHECK(foo->Value == 42);
    CHECK(foo->Name == "foo");

    foo.reset();
    CHECK_FALSE(foo);
}

TEST_CASE("Ref")
{
    Quasar::Ref<Foo> foo = Quasar::CreateRef<Foo>(7, "bar");
    REQUIRE(foo.get() != nullptr);
    CHECK(foo->Value == 7);
    CHECK(foo->Name == "bar");

    Quasar::Ref<Foo> copy = foo;
    CHECK(copy.get() == foo.get());

    foo.reset();
    copy.reset();
    CHECK(copy.get() == nullptr);
}

TEST_CASE("Bit")
{
    CHECK(Quasar::Bit(0) == 1u);
    CHECK(Quasar::Bit(1) == 2u);
    CHECK(Quasar::Bit(5) == 32u);
    CHECK(Quasar::Bit(31) == 0x80000000u);
}

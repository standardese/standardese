// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/cpp_entity_blacklist.hpp>

#include <catch.hpp>

#include "test_parser.hpp"

#include <standardese/doc_entity.hpp>
#include <standardese/generator.hpp>
#include <standardese/index.hpp>
#include <standardese/output.hpp>

using namespace standardese;

TEST_CASE("entity_blacklist")
{
    struct dummy_entity : cpp_entity
    {
        cpp_name name;

        dummy_entity(cpp_name name, cpp_entity::type t)
        : cpp_entity(t, cpp_cursor()), name(std::move(name))
        {
        }

        cpp_name get_name() const override
        {
            return name;
        }
    };

    entity_blacklist blacklist(entity_blacklist::empty);

    dummy_entity ns("ns", cpp_entity::namespace_t);
    dummy_entity type("foo", cpp_entity::class_t);
    dummy_entity variable("foo", cpp_entity::variable_t);

    SECTION("none")
    {
        REQUIRE(!blacklist.is_blacklisted(entity_blacklist::documentation, ns));
        REQUIRE(!blacklist.is_blacklisted(entity_blacklist::documentation, type));
        REQUIRE(!blacklist.is_blacklisted(entity_blacklist::documentation, variable));
        REQUIRE(!blacklist.is_blacklisted(entity_blacklist::synopsis, ns));
        REQUIRE(!blacklist.is_blacklisted(entity_blacklist::synopsis, type));
        REQUIRE(!blacklist.is_blacklisted(entity_blacklist::synopsis, variable));
    }
    SECTION("name-type")
    {
        blacklist.blacklist("ns", cpp_entity::namespace_t);
        REQUIRE(blacklist.is_blacklisted(entity_blacklist::documentation, ns));
        REQUIRE(!blacklist.is_blacklisted(entity_blacklist::documentation, type));
        REQUIRE(!blacklist.is_blacklisted(entity_blacklist::documentation, variable));
        REQUIRE(blacklist.is_blacklisted(entity_blacklist::synopsis, ns));
        REQUIRE(!blacklist.is_blacklisted(entity_blacklist::synopsis, type));
        REQUIRE(!blacklist.is_blacklisted(entity_blacklist::synopsis, variable));
    }
    SECTION("name")
    {
        blacklist.blacklist("foo");
        REQUIRE(!blacklist.is_blacklisted(entity_blacklist::documentation, ns));
        REQUIRE(blacklist.is_blacklisted(entity_blacklist::documentation, type));
        REQUIRE(blacklist.is_blacklisted(entity_blacklist::documentation, variable));
        REQUIRE(!blacklist.is_blacklisted(entity_blacklist::synopsis, ns));
        REQUIRE(blacklist.is_blacklisted(entity_blacklist::synopsis, type));
        REQUIRE(blacklist.is_blacklisted(entity_blacklist::synopsis, variable));
    }
    SECTION("option test")
    {
        REQUIRE(!blacklist.is_set(entity_blacklist::require_comment));
        REQUIRE(!blacklist.is_set(entity_blacklist::extract_private));

        blacklist.set_option(entity_blacklist::require_comment);
        REQUIRE(blacklist.is_set(entity_blacklist::require_comment));
        REQUIRE(!blacklist.is_set(entity_blacklist::extract_private));

        blacklist.set_option(entity_blacklist::extract_private);
        REQUIRE(blacklist.is_set(entity_blacklist::require_comment));
        REQUIRE(blacklist.is_set(entity_blacklist::extract_private));

        blacklist.unset_option(entity_blacklist::require_comment);
        REQUIRE(!blacklist.is_set(entity_blacklist::require_comment));
        REQUIRE(blacklist.is_set(entity_blacklist::extract_private));
    }
}

std::string get_synopsis(const parser& p, const doc_entity& e)
{
    auto              doc = md_document::make("");
    code_block_writer cb(*doc, false);
    e.generate_synopsis(p, cb);
    return static_cast<md_code_block&>(*cb.get_code_block()).get_string();
}

std::string get_synopsis(const parser& p, const doc_file& file, const cpp_entity& e)
{
    // don't support nesting
    for (auto& doc_e : file)
        if (&static_cast<const doc_cpp_entity&>(doc_e).get_cpp_entity() == &e)
            return get_synopsis(p, doc_e);
    REQUIRE(false);
    return "";
}

std::string get_synopsis(const translation_unit& tu, const cpp_entity& e)
{
    standardese::index i;
    auto               file = doc_file::parse(tu.get_parser(), i, "", tu.get_file());
    return get_synopsis(tu.get_parser(), *file, e);
}

std::string get_synopsis(const translation_unit& tu)
{
    standardese::index i;
    auto               file = doc_file::parse(tu.get_parser(), i, "", tu.get_file());
    return get_synopsis(tu.get_parser(), *file);
}

TEST_CASE("synopsis")
{
    parser p(test_logger);
    p.get_output_config().set_flag(output_flag::show_macro_replacement);
    p.get_output_config().set_flag(output_flag::show_complex_noexcept);
    REQUIRE(p.get_output_config().get_tab_width() == 4);

    SECTION("top-level")
    {
        auto code = R"(#define FOO something

constexpr int a(char& c, int* ptr, ...) noexcept(noexcept(1+1));

template <typename T, template <typename> typename D, int ... I>
void b(T t);

template <typename ... T>
void c(T&&... ts);

int var = 32;

using type = int;

template <typename T>
using identity = T;)";

        auto tu = parse(p, "synopsis_toplevel", code);
        REQUIRE(get_synopsis(tu) == code);
    }
    SECTION("namespace")
    {
        auto code = R"(namespace foo
{
    void a();

    /// \exclude
    void b();

    void c();
})";

        auto synopsis = std::string(R"(namespace foo
{
    void a();
____
    void c();
})");
        std::replace(synopsis.begin(), synopsis.end(), '_', ' ');

        auto tu = parse(p, "synopsis_namespace", code);
        REQUIRE(get_synopsis(tu) == synopsis);
    }
    SECTION("class")
    {
        auto code     = R"(
/// foo
class foo
{
public:
    void a();

    foo& operator  =(const foo&);

private:
    void c();

public:
    void d();

protected:
    void e();

public:
    /// \exclude
    void f();

protected:
    void g();

private:
    virtual void h();

    void i();
};)";
        auto synopsis = std::string(R"(class foo
{
public:
    void a();
____
    foo& operator=(const foo&);
____
    void d();
____
protected:
    void e();
____
    void g();
____
private:
    virtual void h();
};)");
        std::replace(synopsis.begin(), synopsis.end(), '_', ' ');

        auto tu = parse(p, "synopsis_class", code);
        REQUIRE(get_synopsis(tu) == "class foo;");
        REQUIRE(get_synopsis(tu, *tu.get_file().begin()) == synopsis);
    }
    SECTION("enum")
    {
        auto code = R"(
/// foo
enum foo: unsigned int
{
    a,
    b = 4,
    c,
    d,

    /// \exclude
    e
};)";
        auto synopsis = R"(enum foo
: unsigned int
{
    a,
    b = 4,
    c,
    d
};)";

        auto tu = parse(p, "synopsis_enum", code);
        REQUIRE(get_synopsis(tu) == "enum foo;");
        REQUIRE(get_synopsis(tu, *tu.get_file().begin()) == synopsis);
    }
    SECTION("function")
    {
        auto code = R"(
/// \param 2
/// \exclude
void func(int a, char* b, float = .3);
)";

        auto synopsis = R"(void func(int a, char* b);)";

        auto tu = parse(p, "synopsis_function", code);
        REQUIRE(get_synopsis(tu) == synopsis);
    }
    SECTION("extensive preprocessor")
    {
        auto code     = R"(
#ifndef BAR
/// \exclude
#define BAR

/// \exclude
#define GENERATE(name) void name(int a);

#ifndef FOO
/// \param a
/// \exclude
GENERATE(foo)
#endif

#endif
)";
        auto synopsis = R"(void foo();)";

        auto tu = parse(p, "synopsis_extensive_preprocessor", code);
        REQUIRE(get_synopsis(tu) == synopsis);
    }
    SECTION("exclusion")
    {
        auto code = R"(
/// \exclude
void foo();

/// \exclude return
int bar();

/// \exclude target
using baz = int;

/// \exclude target
enum : int
{
    a,
};
)";

        auto synopsis = std::string(R"('hidden' bar();

using baz = 'hidden';

enum_
: 'hidden'
{
    a
};)");
        std::replace(synopsis.begin(), synopsis.end(), '_', ' ');

        auto tu = parse(p, "synopsis_exclusion", code);
        REQUIRE(get_synopsis(tu) == synopsis);
    }
}

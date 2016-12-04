// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/template_processor.hpp>

#include <catch.hpp>

#include <standardese/index.hpp>
#include <standardese/output.hpp>

#include "test_parser.hpp"

using namespace standardese;

TEST_CASE("template")
{
    using standardese::index;
    parser p(test_logger);

    auto source = R"(
/// a
struct a {};

/// b
void b();

/// c
struct c
{
    /// f1
    void f1();

    /// f2
    void f2();
};
)";

    auto tu = parse(p, "template_file", source);

    index idx;
    auto  doc = doc_file::parse(p, idx, "file", tu.get_file());

    SECTION("generate_doc_text")
    {
        auto code      = R"(
{{ standardese_doc_text a commonmark }}
)";
        auto generated = R"(
a


)";
        REQUIRE(process_template(p, idx, template_file("template.md", code)).text == generated);
    }
    SECTION("generate_doc_synopsis")
    {
        auto code      = R"(
{{ standardese_doc_synopsis a commonmark }}
)";
        auto generated = R"(
``` cpp
struct a
{
};
```

)";
        REQUIRE(process_template(p, idx, template_file("template.md", code)).text == generated);
    }
    SECTION("generate_doc")
    {
        auto code      = R"(
{{ standardese_doc a commonmark }}
)";
        auto generated = R"(
# Struct `a`<a id="a"></a>

``` cpp
struct a
{
};
```

a


)";
        REQUIRE(process_template(p, idx, template_file("template.md", code)).text == generated);
    }
    SECTION("name")
    {
        auto code = R"(
{{ standardese_name b }},
{{ standardese_unique_name b }},
{{ standardese_index_name b }},
)";
        auto generated = R"(
b,
b(),
b,
)";
        REQUIRE(process_template(p, idx, template_file("template.md", code)).text == generated);
    }
    SECTION("for_each")
    {
        auto code = R"(
{{ standardese_for $entity c }}
    {{ standardese_name $entity }}
{{ standardese_end }}
)";
        auto generated = R"(

    f1

    f2

)";

        REQUIRE(process_template(p, idx, template_file("template.md", code)).text == generated);
    }
    SECTION("if")
    {
        auto code      = R"(
{{ standardese_for $entity template_file }}
{{ standardese_if $entity name a }}
        a_entity
{{ standardese_else_if $entity name b() }}
        b_entity
{{ standardese_else }}
        c_entity
{{ standardese_end }}
{{ standardese_end }}
)";
        auto generated = R"(


        a_entity



        b_entity



        c_entity


)";

        REQUIRE(process_template(p, idx, template_file("template.md", code)).text == generated);
    }
}

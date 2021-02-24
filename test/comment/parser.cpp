// Copyright (C) 2016-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
//               2021 Julian Rüth <julian.rueth@fsfe.org>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <catch.hpp>

#include "../util/assertions/sections.hpp"
#include "../util/indent.hpp"

#include "../../include/standardese/comment/parser.hpp"

namespace standardese::test::comment {

using standardese::comment::parser;
using standardese::test::util::unindent;
using namespace standardese::test::util::assertions;
using standardese::comment::parse_error;


/// Return the result of parsing `comment` as if it were a comment of an entity
/// such as a method.
standardese::comment::parse_result parse(std::string comment) {
    comment = unindent(comment);
    UNSCOPED_INFO(comment);
    return parse(parser(), comment, true);
}


/// Return the result of parsing `comment` as if it were a comment detached
/// from any entity in isolated spot in a file.
standardese::comment::parse_result parse_without_entity(std::string comment) {
    comment = unindent(comment);
    UNSCOPED_INFO(comment);
    return parse(parser(), comment, false);
}


TEST_CASE("Markdown Markup", "[comment]")
{
    SECTION("Markdown Inline Markup is Preserved")
    {
        const auto parsed = parse(R"(
            A brief which is not relevant for this test
    
            This line starts the details.
            `code`
            *emphasis with `code`*\
            **strong emphasis with _emphasis_**
            )");
    
        CHECK_SECTIONS_EQUIVALENT_TO(parsed, R"(
            <details-section>
            <paragraph>This line starts the details.<soft-break></soft-break>
            <code>code</code><soft-break></soft-break>
            <emphasis>emphasis with <code>code</code></emphasis></paragraph>
            <paragraph><strong-emphasis>strong emphasis with <emphasis>emphasis</emphasis></strong-emphasis></paragraph>
            </details-section>
            )");
    }

    SECTION("Markdown Links are Parsed")
    {
        const auto parsed = parse(R"(
            A brief which is not relevant for this test

            [external link](http://foonathan.net)
            [external link `2`](http://standardese.foonathan.net/ "title")
            [internal <link>](<> "name")
            [internal link `2`](standardese://name/ "title")
            [name<T>name]()
            )");

        CHECK_SECTIONS_EQUIVALENT_TO(parsed, R"(
            <details-section>
            <paragraph><external-link url="http://foonathan.net">external link</external-link><soft-break></soft-break>
            <external-link title="title" url="http://standardese.foonathan.net/">external link <code>2</code></external-link><soft-break></soft-break>
            <documentation-link unresolved-destination-id="name">internal &lt;link&gt;</documentation-link><soft-break></soft-break>
            <documentation-link title="title" unresolved-destination-id="name">internal link <code>2</code></documentation-link><soft-break></soft-break>
            <documentation-link unresolved-destination-id="name&lt;T&gt;name"><code>name&lt;T&gt;name</code></documentation-link></paragraph>
            </details-section>
            )");
    }

    SECTION("HTML Markup is not Supported so we can Write vector<T> Without Escaping", "[comment]")
    {
        SECTION("Any Inline HTML is Treated as Text")
        {
            const auto parsed = parse(R"(
                This brief contains <i>some</i> HTML but it won't render as markup so we can write vector<T> without having to escape things.
                )");

            CHECK_BRIEF_EQUIVALENT_TO(parsed, R"(
                <brief-section>This brief contains &lt;i&gt;some&lt;/i&gt; HTML but it won’t render as markup so we can write vector&lt;T&gt; without having to escape things.</brief-section>
                )");
        }
        SECTION("Any HTML Blocks are Treated as Text")
        {
            const auto parsed = parse(R"(
                <p>
                    This paragraph will be part of the brief. But <b>any</b> HTML <p>markup</p> in here will be escaped, even if it is <a><b><i>malformed</b></i>.
                    <p>Also this nested block will be part of the brief.</p>

                A line break ends the preceding block in Markdown. Even though the <p> is still open. So this will be the details and not the brief anymore.
                </p>
                )");

            CHECK_BRIEF_EQUIVALENT_TO(parsed, R"(
                <brief-section>&lt;p&gt;
                    This paragraph will be part of the brief. But &lt;b&gt;any&lt;/b&gt; HTML &lt;p&gt;markup&lt;/p&gt; in here will be escaped, even if it is &lt;a&gt;&lt;b&gt;&lt;i&gt;malformed&lt;/b&gt;&lt;/i&gt;.
                    &lt;p&gt;Also this nested block will be part of the brief.&lt;/p&gt;
                </brief-section>
                )");
            CHECK_SECTIONS_EQUIVALENT_TO(parsed, R"(
                <details-section>
                <paragraph>A line break ends the preceding block in Markdown. Even though the &lt;p&gt; is still open. So this will be the details and not the brief anymore.</paragraph>
                <paragraph>&lt;/p&gt;
                </paragraph>
                </details-section>
                )");
        }
    }

    SECTION("Images are not Supported", "[comment]")
    {
        CHECK_THROWS_AS(parse(R"(![an image](img.png))"), parse_error);
    }

    SECTION("Markdown Quotes are Preserved", "[comment]")
    {
        const auto parsed = parse(R"(
            > This initial quote is not used for the brief but goes into details.
            >
            > A second quote.
            
            > A third quote.
            > The second line of the third quote.
            )");
    
        CHECK_SECTIONS_EQUIVALENT_TO(parsed, R"(
            <details-section>
            <block-quote>
            <paragraph>This initial quote is not used for the brief but goes into details.</paragraph>
            <paragraph>A second quote.</paragraph>
            </block-quote>
            <block-quote>
            <paragraph>A third quote.<soft-break></soft-break>
            The second line of the third quote.</paragraph>
            </block-quote>
            </details-section>
            )");
    }

    SECTION("Markdown Lists are Preserved")
    {
        const auto parsed = parse(R"(
            * This list.
            * is tight.
            
            List break.
            
            * An item with a paragraph.
            
              And another paragraph.
            
            * And a different item.
            
            List break.
            
            1. An
            2. ordered
            3. list
            
            List break.
            
            * A list
            
            * with another
              1. list
              2. inside
            
            * *great*
            )");

        CHECK_SECTIONS_EQUIVALENT_TO(parsed, R"(
            <details-section>
            <unordered-list>
            <list-item>
            <paragraph>This list.</paragraph>
            </list-item>
            <list-item>
            <paragraph>is tight.</paragraph>
            </list-item>
            </unordered-list>
            <paragraph>List break.</paragraph>
            <unordered-list>
            <list-item>
            <paragraph>An item with a paragraph.</paragraph>
            <paragraph>And another paragraph.</paragraph>
            </list-item>
            <list-item>
            <paragraph>And a different item.</paragraph>
            </list-item>
            </unordered-list>
            <paragraph>List break.</paragraph>
            <ordered-list>
            <list-item>
            <paragraph>An</paragraph>
            </list-item>
            <list-item>
            <paragraph>ordered</paragraph>
            </list-item>
            <list-item>
            <paragraph>list</paragraph>
            </list-item>
            </ordered-list>
            <paragraph>List break.</paragraph>
            <unordered-list>
            <list-item>
            <paragraph>A list</paragraph>
            </list-item>
            <list-item>
            <paragraph>with another</paragraph>
            <ordered-list>
            <list-item>
            <paragraph>list</paragraph>
            </list-item>
            <list-item>
            <paragraph>inside</paragraph>
            </list-item>
            </ordered-list>
            </list-item>
            <list-item>
            <paragraph><emphasis>great</emphasis></paragraph>
            </list-item>
            </unordered-list>
            </details-section>
            )");
    }

    SECTION("Markdown Code Blocks are Preserved")
    {
        const auto parsed = parse(R"(
            ```
            A code block starting a comment is not used for a brief but goes to the details.
            ```
            
            ```cpp
            A code block with info.
            ```
            )");

        CHECK_SECTIONS_EQUIVALENT_TO(parsed, R"(
            <details-section>
            <code-block>A code block starting a comment is not used for a brief but goes to the details.
            </code-block>
            <code-block language="cpp">A code block with info.
            </code-block>
            </details-section>
            )");
    }

    SECTION("Markdown Headings are Preserved")
    {
        const auto parsed = parse(R"(
            # A heading starting a comment is not used for a brief but goes to the details.
            
            ## B
            
            ### C
            
            DDD
            ===
            
            EEE
            ---
            )");

        CHECK_SECTIONS_EQUIVALENT_TO(parsed, R"(
            <details-section>
            <heading>A heading starting a comment is not used for a brief but goes to the details.</heading>
            <subheading>B</subheading>
            <subheading>C</subheading>
            <heading>DDD</heading>
            <subheading>EEE</subheading>
            </details-section>
            )");
    }

    SECTION("Markdown Thematic Breaks are Supported", "[comment]")
    {
        const auto parsed = parse(R"(
            This line is used for the brief and does not show up in the details.
            
            A paragraph.
            
            ---
            
            A completely different paragraph.
            )");

        CHECK_SECTIONS_EQUIVALENT_TO(parsed, R"(
            <details-section>
            <paragraph>A paragraph.</paragraph>
            <thematic-break></thematic-break>
            <paragraph>A completely different paragraph.</paragraph>
            </details-section>
            )");
    }

    SECTION("Markdown Paragraphs are (Mostly) Preserved", "[comment]")
    {
        SECTION("Empty Lines Delimit Paragraphs")
        {
            const auto parsed = parse(R"(
                The first line ends in punctuation so it defines the brief and does not show up in the details.
                The first paragraph.
                Second line of the first paragraph.
                
                The second paragraph.
                
                The third paragraph.
                Second line of the third paragraph.
                )");

            CHECK_BRIEF_EQUIVALENT_TO(parsed, R"(
                <brief-section>The first line ends in punctuation so it defines the brief and does not show up in the details.</brief-section>
                )");

            CHECK_SECTIONS_EQUIVALENT_TO(parsed, R"(
                <details-section>
                <paragraph>The first paragraph.<soft-break></soft-break>
                Second line of the first paragraph.</paragraph>
                <paragraph>The second paragraph.</paragraph>
                <paragraph>The third paragraph.<soft-break></soft-break>
                Second line of the third paragraph.</paragraph>
                </details-section>
                )");
        }

        SECTION("Break Paragraphs Between Brief and Details")
        {
            const auto parsed = parse(R"(
                Brief
                sentence
                split
                into
                multiple!
                Not brief.
                )");

            CHECK_BRIEF_EQUIVALENT_TO(parsed, R"(
                <brief-section>Brief<soft-break></soft-break>
                sentence<soft-break></soft-break>
                split<soft-break></soft-break>
                into<soft-break></soft-break>
                multiple!</brief-section>
                )");

            CHECK_SECTIONS_EQUIVALENT_TO(parsed, R"(
                <details-section>
                <paragraph>Not brief.</paragraph>
                </details-section>
                )");
        }

        SECTION("Preserve Complicated Parapgraphs in Details")
        {
            const auto parsed = parse(R"(
                Implicit brief.
                This is not part of the brief anymore because the previous line ends with a full stop.
                
                Still details.
                Even still details.
                
                > Also in quote.
                
                ```
                Or code.
                ```
                
                * Or
                * List
                )");

            CHECK_BRIEF_EQUIVALENT_TO(parsed, R"(
                <brief-section>Implicit brief.</brief-section>
                )");

            CHECK_SECTIONS_EQUIVALENT_TO(parsed, R"(
                <details-section>
                <paragraph>This is not part of the brief anymore because the previous line ends with a full stop.</paragraph>
                <paragraph>Still details.<soft-break></soft-break>
                Even still details.</paragraph>
                <block-quote>
                <paragraph>Also in quote.</paragraph>
                </block-quote>
                <code-block>Or code.
                </code-block>
                <unordered-list>
                <list-item>
                <paragraph>Or</paragraph>
                </list-item>
                <list-item>
                <paragraph>List</paragraph>
                </list-item>
                </unordered-list>
                </details-section>
                )");
        }
    }
}


TEST_CASE("Standardese Specific Markup Rules", "[comment]")
{
    SECTION("Explicit Sections Spanning Multiple Lines")
    {
        const auto parsed = parse(R"(
            \brief Explicit brief.
            Still explicit brief.
             
            \details Explicit details.
             
            Still details.
             
            \effects Explicit effects.
            Still effects.
             
            Details again.
             
            \requires
             
            \returns
            Explicit returns.
            \returns Different returns.
            A backslash at the end of the line causes a hard line break and ends this section.\
            Details again.
            \notes Explicit notes.
            )");

        CHECK_BRIEF_EQUIVALENT_TO(parsed, R"(
            <brief-section>Explicit brief.<soft-break></soft-break>
            Still explicit brief.</brief-section>
            )");

        CHECK_SECTIONS_EQUIVALENT_TO(parsed, {R"(
            <details-section>
            <paragraph>Explicit details.</paragraph>
            <paragraph>Still details.</paragraph>
            <paragraph>Details again.</paragraph>
            <paragraph>Details again.</paragraph>
            </details-section>
            )", R"(
            <inline-section name="Effects">Explicit effects.<soft-break></soft-break>
            Still effects.</inline-section>
            )", R"(
            <inline-section name="Requires"></inline-section>
            )", R"(
            <inline-section name="Return values">Explicit returns.</inline-section>
            )", R"(
            <inline-section name="Return values">Different returns.<soft-break></soft-break>
            A backslash at the end of the line causes a hard line break and ends this section.</inline-section>
            )", R"(
            <inline-section name="Notes">Explicit notes.</inline-section>
            )"});
    }

    SECTION("Section Commands Must Start a Line")
    {
        const auto parsed = parse(R"(
            \details Ignore \effects not starting at beginning.
            Technically, this is because our cmark extension only recognizes such commands when they start a new block, and blocks can only start at the beginning of a line.
            \synopsis Ignore all lines starting with a command.
            But please include me.
            
            > \effects In block quote is ignored.
            
            * \effects In list is ignored.
            )");

        CHECK_SECTIONS_EQUIVALENT_TO(parsed, R"(
            <details-section>
            <paragraph>Ignore \effects not starting at beginning.<soft-break></soft-break>
            Technically, this is because our cmark extension only recognizes such commands when they start a new block, and blocks can only start at the beginning of a line.</paragraph>
            <paragraph>But please include me.</paragraph>
            <block-quote>
            <paragraph>\effects In block quote is ignored.</paragraph>
            </block-quote>
            <unordered-list>
            <list-item>
            <paragraph>\effects In list is ignored.</paragraph>
            </list-item>
            </unordered-list>
            </details-section>
            )");
    }

    SECTION("Sections can be Terminated Explicitly")
    {
        const auto parsed = parse(R"(
            \effects The first line of the effects section.
            The second line of the effects.
            
            This is part of effects and not details because there is an explicit end command.
            
            This is part of effects and not details because there is an explicit end command.
            
            \end
            
            This is the first line of the details.
            
            \returns This is the only line of the returns section.
            \end
            
            \notes This is the first line of the notes section.
            
            Another line of the notes section.
            \end
            This is the second line of the details.
            
            \requires This is the first line of the requires section.
            
            This is the second
            and this is the third line of the requires.\end
            This is the fourth line of the requires section because an \end must be at the start of the line.
            \end
            This is the third line of the details.
            
            \param foo A parameter.
            Going on.
            
            * Still
            * going
            * on
            
            \end
            
            This is the fourth line of the details.
            )");

        CHECK_SECTIONS_EQUIVALENT_TO(parsed, {R"(
            <inline-section name="Effects">The first line of the effects section.<soft-break></soft-break>
            The second line of the effects.<soft-break></soft-break>
            This is part of effects and not details because there is an explicit end command.<soft-break></soft-break>
            This is part of effects and not details because there is an explicit end command.</inline-section>
            )", R"(
            <details-section>
            <paragraph>This is the first line of the details.</paragraph>
            <paragraph>This is the second line of the details.</paragraph>
            <paragraph>This is the third line of the details.</paragraph>
            <paragraph>This is the fourth line of the details.</paragraph>
            </details-section>
            )", R"(
            <inline-section name="Return values">This is the only line of the returns section.</inline-section>
            )", R"(
            <inline-section name="Notes">This is the first line of the notes section.<soft-break></soft-break>
            Another line of the notes section.</inline-section>
            )", R"(
            <inline-section name="Requires">This is the first line of the requires section.<soft-break></soft-break>
            This is the second<soft-break></soft-break>
            and this is the third line of the requires.\end<soft-break></soft-break>
            This is the fourth line of the requires section because an \end must be at the start of the line.</inline-section>
            )"});

        CHECK_INLINES_EQUIVALENT_TO(parsed, R"(
            <details-section>
            <paragraph>Going on.</paragraph>
            <unordered-list>
            <list-item>
            <paragraph>Still</paragraph>
            </list-item>
            <list-item>
            <paragraph>going</paragraph>
            </list-item>
            <list-item>
            <paragraph>on</paragraph>
            </list-item>
            </unordered-list>
            </details-section>
            )");
    }

    SECTION("Key Value Sections (Are No Longer Supported)")
    {
        // Up to 0.6.4, we allowed to write sections of the form:
        // \returns 0 - describe 0
        // 1 - describe 1
        // This output did not work well in the Markdown output and also can be
        // sufficiently well emulated with Markdown.
        // Also this allowed to use the shorthand [link] instead of [link]() on
        // the left hand side of the `-`. This is not supported anymore.
        const auto parsed = parse(R"(
            \returns 0 - Value 0.
            1-Value 1.
            It requires extra long description.
            \returns Default returns.
            \notes This terminates.
            
            \effects -> really weird
            
            \see [foo] - Optional description.
            \see [bar]-
            
            This terminates.
            )");

        CHECK_SECTIONS_EQUIVALENT_TO(parsed, {R"(
            <inline-section name="Return values">0 - Value 0.<soft-break></soft-break>
            1-Value 1.<soft-break></soft-break>
            It requires extra long description.</inline-section>
            )", R"(
            <inline-section name="Return values">Default returns.</inline-section>
            )", R"(
            <inline-section name="Notes">This terminates.</inline-section>
            )", R"(
            <inline-section name="Effects">-&gt; really weird</inline-section>
            )", R"(
            <inline-section name="See also">[foo] - Optional description.</inline-section>
            )", R"(
            <inline-section name="See also">[bar]-</inline-section>
            )", R"(
            <details-section>
            <paragraph>This terminates.</paragraph>
            </details-section>
            )"});
    }
}


TEST_CASE("Standardese Commands", "[comment]")
{
    SECTION(R"(\verbatim Signals Inclusion Without any Markup)")
    {
        const auto parsed = parse(R"(
            \verbatim This is verbatim.\end
            And \verbatim this as well.

            \effects But this is a section.

            With \verbatim VERBATIM<>\end

            \end

            1. A list
               - A child list
                 \verbatim #105\end
            )");

        CHECK_BRIEF_EQUIVALENT_TO(parsed, R"(
            <brief-section><verbatim>This is verbatim.</verbatim><soft-break></soft-break>
            And <verbatim>this as well.</verbatim></brief-section>
            )");

        CHECK_SECTIONS_EQUIVALENT_TO(parsed, std::vector<std::string>{R"(
            <inline-section name="Effects">But this is a section.<soft-break></soft-break>
            With <verbatim>VERBATIM&lt;&gt;</verbatim></inline-section>
            )", R"(
            <details-section>
            <ordered-list>
            <list-item>
            <paragraph>A list</paragraph>
            <unordered-list>
            <list-item>
            <paragraph>A child list<soft-break></soft-break>
            <verbatim>#105</verbatim></paragraph>
            </list-item>
            </unordered-list>
            </list-item>
            </ordered-list>
            </details-section>
            )"});
    }

    SECTION(R"(\exclude Overrides Which Parts of a Node Should be Included)")
    {
        const auto parse_exclude = [](const std::string& comment) {
            const auto parsed = parse(comment);
            REQUIRE(parsed.comment.has_value());
            return parsed.comment.value().metadata().exclude();
        };

        SECTION("The Default is not to Exclude Anything")
        {
            CHECK(parse_exclude("a comment") == type_safe::nullopt);
        }
        SECTION("The Command Works Without Arguments or With Special Keywords")
        {
            CHECK(parse_exclude(R"(\exclude)") == standardese::comment::exclude_mode::entity);
            CHECK(parse_exclude(R"(\exclude target)") == standardese::comment::exclude_mode::target);
            CHECK(parse_exclude(R"(\exclude return)") == standardese::comment::exclude_mode::return_type);
            
        }
        SECTION("With other Keywords, the command is treated as text.")
        {
            const auto parsed = parse(R"(\exclude foo)");
            CHECK_BRIEF_EQUIVALENT_TO(parsed, R"(
            <brief-section>\exclude foo</brief-section>
            )");
        }

        SECTION("Multiple Excludes are not Allowed")
        {
            CHECK_THROWS_AS(parse_exclude("\\exclude\n\\exclude"), parse_error);
        }
    }

    SECTION(R"(\unique_name Command Overrides the Name of an Entity for Linking)")
    {
        const auto parse_unique_name = [](const std::string& comment) {
            const auto parsed = parse(comment);
            REQUIRE(parsed.comment.has_value());
            return parsed.comment.value().metadata().unique_name();
        };

        SECTION("The Default is not to set the Unique Name")
        {
            CHECK(parse_unique_name("a comment") == type_safe::nullopt);
        }
        SECTION("A Unique Name can be Set Explicitly")
        {
            CHECK(parse_unique_name(R"(\unique_name name)") == "name");
        }
        SECTION("Malformed Unique Names are Treated as Text")
        {
            const auto parsed = parse(R"(\unique_name a b c)");
            CHECK_BRIEF_EQUIVALENT_TO(parsed, R"(
            <brief-section>\unique_name a b c</brief-section>
            )");
        }
        SECTION("There can only be one Unique Name")
        {
            CHECK_THROWS_AS(parse_unique_name(R"(
              \unique_name a
              \unique_name b
              )"), parse_error);
        }
    }

    SECTION(R"(\output_name Changes the Name of an Entity in the Generated Output)")
    {
        const auto parse_output_name = [](const std::string& comment) {
            const auto parsed = parse(comment);
            REQUIRE(parsed.comment.has_value());
            return parsed.comment.value().metadata().output_name();
        };

        SECTION("The Default is not to set the Output Name")
        {
            CHECK(parse_output_name("a comment") == type_safe::nullopt);
        }
        SECTION("The Output Name can be set Explicitly")
        {
            CHECK(parse_output_name(R"(\output_name name)") == "name");
        }
        SECTION("A Malformed Output Name is Treated as Text")
        {
            const auto parsed = parse(R"(\output_name a b c)");
            CHECK_BRIEF_EQUIVALENT_TO(parsed, R"(
            <brief-section>\output_name a b c</brief-section>
            )");
        }
        SECTION("There can only be one Output Name")
        {
            CHECK_THROWS_AS(parse_output_name(R"(
                \output_name a
                \output_name b
                )"), parse_error);
        }
    }

    SECTION(R"(\synopsis Completely Overrides the Synopsis of an Entity)")
    {
        const auto parse_synopsis = [](const std::string& comment) {
            const auto parsed = parse(comment);
            REQUIRE(parsed.comment.has_value());
            return parsed.comment.value().metadata().synopsis();
        };

        SECTION("The Default is not to set the Synopsis Override")
        {
            CHECK(parse_synopsis("a comment") == type_safe::nullopt);
        }
        SECTION("Synopsis can be set to any String")
        {
            CHECK(parse_synopsis(R"(\synopsis void f())") == "void f()");
            CHECK(parse_synopsis(R"(\synopsis)") == "");
        }
        SECTION("The Synopsis must be Unique and Non-Empty")
        {
            CHECK_THROWS_AS(parse_synopsis(R"(
                \synopsis a
                \synopsis b
                )"), parse_error);
        }
    }

    SECTION(R"(\group and \output_section Groups Members Together)")
    {
        const auto parse_metadata = [](const std::string& comment) {
            const auto parsed = parse(comment);
            REQUIRE(parsed.comment.has_value());
            return parsed.comment.value().metadata();
        };

        SECTION("The Default is not to set Group and Output Section")
        {
            CHECK(parse_metadata("a comment").group() == type_safe::nullopt);
            CHECK(parse_metadata("a comment").output_section() == type_safe::nullopt);
        }
        SECTION("Group can be set Explicitly")
        {
            const auto metadata = parse_metadata(R"(
                \group name
            )");
            REQUIRE(metadata.group().has_value());
            CHECK(metadata.group().value().name() == "name");
            CHECK(!metadata.group().value().heading().has_value());
        }
        SECTION("Group can be set Explicitly Without Defining an Output Section")
        {
            const auto metadata = parse_metadata(R"(
                \group -name
            )");
            REQUIRE(metadata.group().has_value());
            CHECK(metadata.group().value().name() == "name");
            CHECK(!metadata.group().value().heading().has_value());
            CHECK(!metadata.output_section().has_value());
        }
        SECTION("Group can be set Explicitly With a Specific Heading")
        {
            const auto metadata = parse_metadata(R"(
                \group name Group Heading
            )");
            REQUIRE(metadata.group().has_value());
            CHECK(metadata.group().value().name() == "name");
            CHECK(metadata.group().value().heading() == "Group Heading");
            CHECK(!metadata.output_section().has_value());
        }
        SECTION("Output Section can be set Explicitly")
        {
            const auto metadata = parse_metadata(R"(
                \output_section name
            )");
            REQUIRE(!metadata.group().has_value());
            CHECK(metadata.output_section() == "name");
        }
        SECTION("Malformed Commands are Treated as Text")
        {
            CHECK_BRIEF_EQUIVALENT_TO(parse(R"(
                \group
                \group
                )"), R"(
                <brief-section>\group<soft-break></soft-break>
                \group</brief-section>
                )");
        }
        SECTION("Only one of Group and Output Section can be set Exactly Once")
        {
            CHECK_THROWS_AS(parse_metadata(R"(
                \group a
                \group a
                )"), parse_error);
            CHECK_THROWS_AS(parse(R"(
                \output_section
                \output_section
                )"), parse_error);
            CHECK_THROWS_AS(parse_metadata(R"(
                \group a
                \output_section a
                )"), parse_error);
            CHECK_THROWS_AS(parse_metadata(R"(
                \output_section a
                \output_section a
                )"), parse_error);
        }
    }

    SECTION(R"(\module Groups Entities Across File Boundaries)")
    {
        const auto parse_module = [](const std::string& comment) {
            const auto parsed = parse(comment);
            REQUIRE(parsed.comment.has_value());
            return parsed.comment.value().metadata().module();
        };

        SECTION("The Default is not to Assign any Module")
        {
            CHECK(parse_module("a comment") == type_safe::nullopt);
        }
        SECTION("The Module can be set Explicitly")
        {
            CHECK(parse_module(R"(\module name)") == "name");
        }
        SECTION("A Malformed Command is Treated as Text")
        {
            CHECK_BRIEF_EQUIVALENT_TO(parse(R"(
                \module
                )"), R"(
                <brief-section>\module</brief-section>
                )");
            CHECK_BRIEF_EQUIVALENT_TO(parse(R"(
                \module a b c
                )"), R"(
                <brief-section>\module a b c</brief-section>
                )");
        }
        SECTION("The Module must be Unique")
        {
            CHECK_THROWS_AS(parse_module(R"(
                \module a
                \module b
                )"), parse_error);
        }
        SECTION(R"(A \module Command at the File Level Provides Documentation for the Module Itself)")
        {
            const auto parsed = parse_without_entity(R"(
                \module M
                Description of the module M
                )");
            REQUIRE(parsed.entity.has_value());
            CHECK(parsed.entity.value(type_safe::variant_type<standardese::comment::module>{}).name == "M");
        }
    }

    SECTION(R"(The \file Command Provides Description for an Entire Header File)")
    {
        SECTION(R"(A \file Command can Appear at the Top-Level, i.e., not Associated to an Entity)")
        {
            const auto parsed = parse_without_entity(R"(\file description")");
            CHECK(is_file(parsed.entity));
        }
        SECTION(R"(A \file Command cannot Appear next to an Entity such as a Method)")
        {
            CHECK_THROWS_AS(parse(R"(\file description)"), parse_error);
        }
    }

    SECTION(R"(The \entity Command Provides Documentation for Another Entity)")
    {
        SECTION("The Entity is Never set Implicitly")
        {
            CHECK(parse("a comment").entity == type_safe::nullvar);
            CHECK(parse_without_entity("a comment").entity == type_safe::nullvar);
        }
        SECTION("The Entity can be set Explicitly")
        {
            CHECK(get_remote_entity(parse_without_entity(R"(
                \entity f()
                This documentation is for the function f()
                )").entity) == "f()");
        }
        SECTION("The Implicit Entity Cannot be Replaced")
        {
            CHECK_THROWS_AS(parse(R"(
                \entity f()
                This documentation is for the function f() and not for the entity which is next to.
                )"), parse_error);
        }
        SECTION("The Entity must be Unique and Non-Empty")
        {
            CHECK_THROWS_AS(parse_without_entity(R"(
                \entity a
                \entity b
                )"), parse_error);
            CHECK_THROWS_AS(parse_without_entity(R"(
                \entity a
                \file
                )"), parse_error);
        }
    }

    SECTION(R"(Unknown Commands are Reproduced Verbatim)")
    {
        CHECK_BRIEF_EQUIVALENT_TO(parse(R"(
            \invalid comment
            )"), R"(
            <brief-section>\invalid comment</brief-section>
            )");
    }

    SECTION(R"(The \param Command Describes a Parameter of a Function or Method)")
    {
        SECTION("A Malformed Command is Reproduced as Text")
        {
            CHECK_BRIEF_EQUIVALENT_TO(parse(R"(
                \param
                )"), R"(
                <brief-section>\param</brief-section>
                )");
        }
        SECTION(R"(A \param Follows the Usual Parsing Rules: the First Line is Brief, the Rest are Details)")
        {
            const auto parsed = parse(R"(
                This is the brief of the method we are documenting.
                These are the details of the method we are documenting.
                \param a This is the brief of the parameter a.
                These are the details of the parameter a.
                )");

            CHECK_BRIEF_EQUIVALENT_TO(parsed, R"(
                <brief-section>This is the brief of the method we are documenting.</brief-section>
                )");
            CHECK_SECTIONS_EQUIVALENT_TO(parsed, R"(
                <details-section>
                <paragraph>These are the details of the method we are documenting.</paragraph>
                </details-section>
                )");
            CHECK_BRIEF_EQUIVALENT_TO(parsed.inlines.at(0), R"(
                <brief-section>This is the brief of the parameter a.</brief-section>
                )");
            CHECK_SECTIONS_EQUIVALENT_TO(parsed.inlines.at(0), R"(
                <details-section>
                <paragraph>These are the details of the parameter a.</paragraph>
                </details-section>
                )");
        }
        SECTION(R"(A \param ends Implicitly when Another Section Begins")")
        {
            const auto parsed = parse(R"(
                \param a This is the brief of the parameter a.
                \returns This is the description of the return value.
                )");
            CHECK_BRIEF_EQUIVALENT_TO(parsed.inlines.at(0), R"(
                <brief-section>This is the brief of the parameter a.</brief-section>
                )");
            CHECK_SECTIONS_EQUIVALENT_TO(parsed.inlines.at(0), std::vector<std::string>{});

        }
        SECTION(R"(A \param ends Implicitly when a Paragraph Ends")")
        {
            const auto parsed = parse(R"(
                \param a This is the brief of the parameter a.
                These are the details of the parameter a.

                These are the details of the method.
                )");

            CHECK_SECTIONS_EQUIVALENT_TO(parsed, R"(
                <details-section>
                <paragraph>These are the details of the method.</paragraph>
                </details-section>
                )");
            CHECK_BRIEF_EQUIVALENT_TO(parsed.inlines.at(0), R"(
                <brief-section>This is the brief of the parameter a.</brief-section>
                )");
            CHECK_SECTIONS_EQUIVALENT_TO(parsed.inlines.at(0), R"(
                <details-section>
                <paragraph>These are the details of the parameter a.</paragraph>
                </details-section>
                )");
        }
        SECTION(R"(A \param can be Ended Explicitly with an \end)")
        {
            const auto parsed = parse(R"(
                \param a This is the brief of the parameter a.
                \end
                These are the details of the method.
                )");

            CHECK_SECTIONS_EQUIVALENT_TO(parsed, R"(
                <details-section>
                <paragraph>These are the details of the method.</paragraph>
                </details-section>
                )");
            CHECK_BRIEF_EQUIVALENT_TO(parsed.inlines.at(0), R"(
                <brief-section>This is the brief of the parameter a.</brief-section>
                )");
            CHECK_SECTIONS_EQUIVALENT_TO(parsed.inlines.at(0), std::vector<std::string>{});
        }
    }
    SECTION("Section Commands")
    {
        // We should test all the section commands here.
    }
}

}

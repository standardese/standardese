// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_MARKUP_GENERATOR_HPP_INCLUDED
#define STANDARDESE_MARKUP_GENERATOR_HPP_INCLUDED

#include <iosfwd>
#include <functional>
#include <string>

namespace standardese
{
    namespace markup
    {
        class entity;

        /// A generator.
        ///
        /// It will write the entity representation to the given stream.
        using generator = std::function<void(std::ostream&, const entity&)>;

        /// Renders an entity to a string.
        ///
        /// \returns The string representation of the entity in the given format.
        std::string render(generator gen, const entity& e);

        /// An HTML generator.
        ///
        /// \returns A generator that will generate the HTML representation.
        generator html_generator(const std::string& extension = "html") noexcept;

        /// Renders an entity as HTML.
        ///
        /// \returns `render(html_generator(), e)`.
        inline std::string as_html(const entity& e)
        {
            return render(html_generator(), e);
        }

        /// A markdown generator.
        ///
        /// \returns A generator that will generate a CommonMark representation.
        /// If `use_html` is `true`, it will use HTML for complex parts that cannot be described using CommonMark.
        generator markdown_generator(bool use_html = true) noexcept;

        /// Renders an entity as CommonMark.
        ///
        /// \returns `render(commonmark_generator(), e)`.
        inline std::string as_markdown(const entity& e)
        {
            return render(markdown_generator(), e);
        }

        /// An XML generator.
        ///
        /// It will use a simple XML format to describe the markup AST.
        ///
        /// \returns A generator that will generate the XML representation.
        generator xml_generator(bool include_attributes = true) noexcept;

        /// Renders an entity as XML.
        ///
        /// \returns `render(xml_generator(), e)`.
        inline std::string as_xml(const entity& e)
        {
            return render(xml_generator(), e);
        }
    }
} // namespace standardese::markup

#endif // STANDARDESE_MARKUP_GENERATOR_HPP_INCLUDED

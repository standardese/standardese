// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_MARKUP_GENERATOR_HPP_INCLUDED
#define STANDARDESE_MARKUP_GENERATOR_HPP_INCLUDED

#include <iosfwd>
#include <string>

namespace standardese
{
    namespace markup
    {
        class entity;

        /// A generator.
        ///
        /// It will write the entity representation to the given stream.
        using generator = void (*)(std::ostream&, const entity&);

        /// Renders an entity to a string.
        ///
        /// \returns The string representation of the entity in the given format.
        std::string render(generator gen, const entity& e);

        /// An HTML generator.
        ///
        /// \returns A generator that will generate the HTML representation.
        generator html_generator() noexcept;

        /// Renders an entity as HTML.
        ///
        /// \returns `render(html_generator(), e)`.
        inline std::string as_html(const entity& e)
        {
            return render(html_generator(), e);
        }
    }
} // namespace standardese::markup

#endif // STANDARDESE_MARKUP_GENERATOR_HPP_INCLUDED

// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/detail/synopsis_utils.hpp>

using namespace standardese;

void detail::write_type_value_default(output_base::code_block_writer &out,
                              const cpp_type_ref &type, const cpp_name &name,
                              const std::string &def)
{
    if (!name.empty())
    {
        auto &type_name = type.get_name();
        auto pos = type_name.find("(*");
        if (pos != std::string::npos)
        {
            for (auto i = 0u; i <= pos + 1; ++i)
                out << type_name[i];
            out << name;
            for (auto i = pos + 2; i != type_name.size(); ++i)
                out << type_name[i];
        }
        else
        {
            pos = type_name.find('[');
            if (pos != std::string::npos)
            {
                for (auto i = 0u; i != pos; ++i)
                    out << type_name[i];
                out << ' ' << name;
                for (auto i = pos; i != type_name.size(); ++i)
                    out << type_name[i];
            }
            else
                out << type_name << ' ' << name;
        }
    }
    else
        out << type.get_name();

    if (!def.empty())
        out << " = " << def;
}

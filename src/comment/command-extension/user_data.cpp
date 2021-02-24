// Copyright (C) 2021 Julian Rüth <julian.rueth@fsfe.org>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "user_data.hpp"

#include "../cmark-extension/cmark_extension.hpp"
#include "command_extension.hpp"
#include <stdexcept>
#include <cassert>

namespace standardese::comment::command_extension
{

template <typename T>
void user_data<T>::set(cmark_node* node, T command, std::vector<std::string> arguments)
{
    using cmark = cmark_extension::cmark_extension;

    if (cmark_node_get_type(node) != command_extension::node_type<T>())
        throw std::invalid_argument("Cannot associate this kind of user data to the node " + cmark::to_xml(node));

    auto* data = new user_data();
    data->command = command;
    data->arguments_ = std::move(arguments);

    cmark::cmark_node_set_user_data(node, data);
    cmark::cmark_node_set_user_data_free_func(node, [](cmark_mem*, void* data) {
        delete static_cast<user_data*>(data);
    });
}

template <typename T>
const user_data<T>& user_data<T>::get(cmark_node* node)
{
    using cmark = cmark_extension::cmark_extension;

    if (cmark_node_get_type(node) != command_extension::node_type<T>())
        throw std::invalid_argument("Cannot retrieve this kind of user data from the node " + cmark::to_xml(node));

    return *static_cast<user_data*>(cmark_node_get_user_data(node));
}

template <typename T>
std::string user_data<T>::argument(size_t i, size_t count) const {
    std::string value;
    for (size_t k = i; k < arguments_.size(); k += count)
    {
        if (arguments_[k] != "") {
            assert(value == "" && "multiple values for the same argument found; this likely means that a command's regular expression is malformed");
            value = arguments_[k];
        }
    }
    return value;
}

}

namespace standardese::comment::command_extension {

template class user_data<command_type>;
template class user_data<section_type>;
template class user_data<inline_type>;

}

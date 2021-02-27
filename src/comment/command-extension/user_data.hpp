// Copyright (C) 2021 Julian Rüth <julian.rueth@fsfe.org>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <tuple>
#include <vector>
#include <string>

#include <cmark-gfm.h>

namespace standardese::comment::command_extension
{
    /// Holds the Arguments Associated to a CommonMark node Created by this Extension.
    template <typename T>
    class user_data
    {
      public:
        /// Associate `command` and `arguments` with this `node`.
        static void set(cmark_node* node, T command, std::vector<std::string> arguments);

        /// Retrieve the command and its arguments from this `node`.
        static const user_data& get(cmark_node* node);
        
        /// The command from which the parser created this node.
        T command;

        /// Return a tuple containing the arguments of the command from which this node was created.
        template <size_t count>
        auto arguments() const {
            return arguments(std::make_index_sequence<count>());    
        }

      private:
        /// Return a tuple containing the arguments of the command from which this node was created.
        template <size_t... I>
        auto arguments(std::index_sequence<I...>) const {
            return std::make_tuple(argument(I, sizeof...(I))...);
        }

        /// Return the value of the argument `i` (assuming that there are
        /// `count` arguments in total.)
        std::string argument(size_t i, size_t count) const;

        std::vector<std::string> arguments_;
    };
}

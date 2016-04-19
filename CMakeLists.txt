# Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

cmake_minimum_required(VERSION 3.1)
project(STANDARDESE VERSION 0.0)

option(STANDARDESE_BUILD_TOOL "whether or not to build the tool" ON)

# subdirectories
add_subdirectory(src)
if(STANDARDESE_BUILD_TOOL)
    add_subdirectory(tool)
endif()



# Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

set(header )
set(src main.cpp)

add_executable(standardese ${header} ${src})
target_link_libraries(standardese PUBLIC standardese_library)

# Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

set(header_path ${STANDARDESE_SOURCE_DIR}/include/standardese)
set(detail_header )
set(header )
set(src )

add_library(standardese_library ${detail_header} ${header} ${src})
comp_target_features(standardese_library PUBLIC
                     cpp11_lang/noexcept
                     cpp14_lib/make_unique
                     PREFIX "STANDARDESE_" NAMESPACE "standardese_comp")
target_include_directories(standardese_library PUBLIC
                                $<BUILD_INTERFACE:${STANDARDESE_SOURCE_DIR}/include>)
target_compile_definitions(standardese_library PUBLIC
                                STANDARDESE=1
                                STANDARDESE_VERSION_MAJOR=${STANDARDESE_VERSION_MAJOR}
                                STANDARDESE_VERSION_MINOR=${STANDARDESE_VERSION_MINOR})

# link libclang
target_include_directories(standardese_library PUBLIC ${LIBCLANG_INCLUDE_DIR})
target_link_libraries(standardese_library PUBLIC ${LIBCLANG_LIBRARY})

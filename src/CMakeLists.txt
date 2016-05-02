# Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

set(detail_header
        ../include/standardese/detail/parse_utils.hpp
        ../include/standardese/detail/search_token.hpp
        ../include/standardese/detail/wrapper.hpp)
set(header
        ../include/standardese/cpp_cursor.hpp
        ../include/standardese/cpp_entity.hpp
        ../include/standardese/cpp_class.hpp
        ../include/standardese/cpp_enum.hpp
        ../include/standardese/cpp_function.hpp
        ../include/standardese/cpp_namespace.hpp
        ../include/standardese/cpp_preprocessor.hpp
        ../include/standardese/cpp_template.hpp
        ../include/standardese/cpp_type.hpp
        ../include/standardese/cpp_variable.hpp
        ../include/standardese/parser.hpp
        ../include/standardese/string.hpp
        ../include/standardese/translation_unit.hpp)
set(src
        detail/parse_utils.cpp
        cpp_class.cpp
        cpp_enum.cpp
        cpp_function.cpp
        cpp_namespace.cpp
        cpp_preprocessor.cpp
        cpp_template.cpp
        cpp_type.cpp
        cpp_variable.cpp
        parser.cpp
        translation_unit.cpp)

add_library(standardese_library ${detail_header} ${header} ${src})
comp_target_features(standardese_library PUBLIC
                     cpp11_lang/noexcept
                     PREFIX "STANDARDESE_" NAMESPACE "standardese_comp")
target_include_directories(standardese_library PUBLIC
                                $<BUILD_INTERFACE:${STANDARDESE_SOURCE_DIR}/include>)
target_compile_definitions(standardese_library PUBLIC
                                STANDARDESE=1
                                STANDARDESE_VERSION_MAJOR=${STANDARDESE_VERSION_MAJOR}
                                STANDARDESE_VERSION_MINOR=${STANDARDESE_VERSION_MINOR}
                                LIBCLANG_SYSTEM_INCLUDE_DIR="${LIBCLANG_SYSTEM_INCLUDE_DIR}/clang/3.7.1/include")

# link libclang
target_include_directories(standardese_library PUBLIC ${LIBCLANG_INCLUDE_DIR})
target_link_libraries(standardese_library PUBLIC ${LIBCLANG_LIBRARY})

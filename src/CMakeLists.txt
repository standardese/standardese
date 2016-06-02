# Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

set(detail_header
        ../include/standardese/detail/parse_utils.hpp
        ../include/standardese/detail/scope_stack.hpp
        ../include/standardese/detail/sequence_stream.hpp
        ../include/standardese/detail/synopsis_utils.hpp
        ../include/standardese/detail/tokenizer.hpp
        ../include/standardese/detail/wrapper.hpp)
set(header
        ../include/standardese/comment.hpp
        ../include/standardese/cpp_class.hpp
        ../include/standardese/cpp_cursor.hpp
        ../include/standardese/cpp_entity.hpp
        ../include/standardese/cpp_entity_registry.hpp
        ../include/standardese/cpp_enum.hpp
        ../include/standardese/cpp_function.hpp
        ../include/standardese/cpp_namespace.hpp
        ../include/standardese/cpp_preprocessor.hpp
        ../include/standardese/cpp_template.hpp
        ../include/standardese/cpp_type.hpp
        ../include/standardese/cpp_variable.hpp
        ../include/standardese/error.hpp
        ../include/standardese/generator.hpp
        ../include/standardese/output.hpp
        ../include/standardese/parser.hpp
        ../include/standardese/string.hpp
        ../include/standardese/synopsis.hpp
        ../include/standardese/translation_unit.hpp)
set(src
        detail/parse_utils.cpp
        detail/scope_stack.cpp
        detail/synopsis_utils.cpp
        detail/tokenizer.cpp
        comment.cpp
        config.cpp
        cpp_class.cpp
        cpp_entity.cpp
        cpp_enum.cpp
        cpp_function.cpp
        cpp_namespace.cpp
        cpp_preprocessor.cpp
        cpp_template.cpp
        cpp_type.cpp
        cpp_variable.cpp
        error.cpp
        generator.cpp
        output.cpp
        output_stream.cpp
        parser.cpp
        synopsis.cpp
        translation_unit.cpp)

add_library(standardese_library ${detail_header} ${header} ${src})
comp_target_features(standardese_library PUBLIC
                     cpp11_lang/noexcept
                     PREFIX "STANDARDESE_" NAMESPACE "standardese_comp")
target_include_directories(standardese_library PUBLIC
                                $<BUILD_INTERFACE:${STANDARDESE_SOURCE_DIR}/include>
                                $<BUILD_INTERFACE:${SPDLOG_INCLUDE_DIR}>)
target_compile_definitions(standardese_library PUBLIC
                                STANDARDESE=1
                                STANDARDESE_VERSION_MAJOR=${STANDARDESE_VERSION_MAJOR}
                                STANDARDESE_VERSION_MINOR=${STANDARDESE_VERSION_MINOR}
                                LIBCLANG_SYSTEM_INCLUDE_DIR="${LIBCLANG_SYSTEM_INCLUDE_DIR}")

# add threading support
find_package(Threads REQUIRED)
target_link_libraries(standardese_library PUBLIC Threads::Threads)

# link libclang
target_include_directories(standardese_library PUBLIC ${LIBCLANG_INCLUDE_DIR})
target_link_libraries(standardese_library PUBLIC ${LIBCLANG_LIBRARY})

# link Boost
find_package(Boost COMPONENTS wave REQUIRED)
target_include_directories(standardese_library PUBLIC ${Boost_INCLUDE_DIR})
target_link_libraries(standardese_library PUBLIC ${Boost_LIBRARIES})


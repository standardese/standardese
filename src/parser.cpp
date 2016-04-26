// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/parser.hpp>

#include <mutex>
#include <set>
#include <vector>

#include <standardese/cpp_namespace.hpp>
#include <standardese/translation_unit.hpp>

using namespace standardese;

const char* const cpp_standard::cpp_98 = "-std=c++98";
const char* const cpp_standard::cpp_03 = "-std=c++03";
const char* const cpp_standard::cpp_11 = "-std=c++11";
const char* const cpp_standard::cpp_14 = "-std=c++14";

struct parser::impl
{
    std::mutex file_mutex;
    std::vector<cpp_file*> files;

    std::mutex ns_mutex;
    std::vector<cpp_namespace*> namespaces;
    std::set<cpp_name> namespace_names;
};

parser::parser()
: index_(clang_createIndex(1, 1)), pimpl_(new impl)
{}

parser::~parser() STANDARDESE_NOEXCEPT {}

translation_unit parser::parse(const char *path, const char *standard) const
{
    const char* args[] = {"-x", "c++", standard};

    auto tu = clang_parseTranslationUnit(index_.get(), path, args, 3, nullptr, 0,
                                         CXTranslationUnit_Incomplete | CXTranslationUnit_DetailedPreprocessingRecord);

    return translation_unit(*this, tu, path);
}

void parser::register_file(cpp_file &f) const
{
    std::unique_lock<std::mutex> lock(pimpl_->file_mutex);
    pimpl_->files.push_back(&f);
}

void parser::for_each_file(file_callback cb, void* data)
{
    for (auto ptr : pimpl_->files)
        cb(*ptr, data);
}

void parser::register_namespace(cpp_namespace &n) const
{
    std::unique_lock<std::mutex> lock(pimpl_->ns_mutex);
    pimpl_->namespace_names.insert(n.get_unique_name());
    pimpl_->namespaces.push_back(&n);
}

void parser::for_each_namespace(namespace_callback cb, void *data)
{
    for (auto& n : pimpl_->namespace_names)
        cb(n, data);
}

const cpp_namespace* parser::for_each_in_namespace(const cpp_name &n, in_namespace_callback cb, void *data)
{
    const cpp_namespace* res = nullptr;
    for (auto& ns : pimpl_->namespaces)
    {
        if (ns->get_name() != n)
            continue;
        res = ns;
        for (auto& e : *ns)
            cb(e, data);
    }

    return res;
}

void parser::deleter::operator()(CXIndex idx) const STANDARDESE_NOEXCEPT
{
    clang_disposeIndex(idx);
}

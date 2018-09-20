// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_THREAD_POOL_HPP_INCLUDED
#define STANDARDESE_THREAD_POOL_HPP_INCLUDED

#include <future>
#include <thread>
#include <vector>

#include <ThreadPool.h>

namespace standardese_tool
{
using thread_pool = ::ThreadPool;

inline unsigned default_no_threads()
{
    return std::max(std::thread::hardware_concurrency(), 1u);
}

template <typename Fnc, typename... Args>
auto add_job(thread_pool& p, Fnc f, Args&&... args)
    -> std::future<typename std::result_of<Fnc(Args...)>::type>
{
    return p.enqueue(f, std::forward<Args>(args)...);
}
} // namespace standardese_tool

#endif // STANDARDESE_THREAD_POOL_HPP_INCLUDED

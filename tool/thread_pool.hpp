// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_THREAD_POOL_HPP_INCLUDED
#define STANDARDESE_THREAD_POOL_HPP_INCLUDED

#include <future>
#include <vector>
#include <thread>

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

    template <typename Container, typename Predicate, typename Func>
    auto for_each(thread_pool& pool, const Container& cont, const Predicate& p, const Func& f) ->
        typename std::enable_if<std::is_same<decltype(f(cont[0])), void>::value>::type
    {
        std::vector<std::future<void>> futures;
        futures.reserve(cont.size());

        for (auto& elem : cont)
            if (p(elem))
                futures.push_back(add_job(pool, f, std::ref(elem)));

        for (auto& future : futures)
            future.wait();
    }

    template <typename Container, typename Predicate, typename Func>
    auto for_each(thread_pool& pool, const Container& cont, const Predicate& p, const Func& f) ->
        typename std::enable_if<!std::is_same<decltype(f(cont[0])), void>::value,
                                std::vector<decltype(f(cont[0]))>>::type
    {
        std::vector<std::future<decltype(f(cont[0]))>> futures;
        futures.reserve(cont.size());

        for (auto& elem : cont)
            if (p(elem))
                futures.push_back(add_job(pool, f, std::ref(elem)));

        std::vector<decltype(f(cont[0]))> results;
        results.reserve(futures.size());
        for (auto& future : futures)
            results.push_back(future.get());

        return results;
    }
} // namespace standardese_tool

#endif // STANDARDESE_THREAD_POOL_HPP_INCLUDED

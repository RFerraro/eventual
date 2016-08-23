#pragma once

#include "eventual_methods.h"
#include <exception>

#include "future.h"
#include "detail/detail.h"
#include "detail/FutureHelper.h"
#include "detail/traits.h"

#include "eventual_methods.cpp"

namespace eventual
{
    template<class T>
    future<detail::decay_future_result_t<T>>
        make_ready_future(T&& value)
    {
        using result_t = detail::decay_future_result_t<T>;
        promise<result_t> promise;
        promise.set_value(std::forward<result_t>(value));
        return promise.get_future();
    }

    template<class T>
    future<T> make_exceptional_future(std::exception_ptr ex)
    {
        promise<T> promise;
        promise.set_exception(ex);
        return promise.get_future();
    }

    template<class T, class E>
    future<T> make_exceptional_future(E ex)
    {
        promise<T> promise;
        promise.set_exception(std::make_exception_ptr(ex));
        return promise.get_future();
    }

    template<class InputIterator>
    detail::all_futures_vector<InputIterator>
        when_all(InputIterator first, InputIterator last)
    {
        using namespace std;
        using future_vector_t = vector<typename iterator_traits<InputIterator>::value_type>;

        auto vectorfuture = make_ready_future(future_vector_t());

        for (auto iterator = first; iterator != last; ++iterator)
        {
            auto& future = *iterator;
            vectorfuture = vectorfuture.then(
                [future = move(future)](auto& vf) mutable
            {
                return future.then(
                    [vf = move(vf)](auto& future) mutable
                {
                    auto resultVector = vf.get();
                    resultVector.emplace_back(move(future));
                    return resultVector;
                });
            });
        }

        return vectorfuture;
    }

    template<class... Futures>
    detail::all_futures_tuple<Futures...>
        when_all(Futures&&... futures)
    {
        return detail::When_All_(std::forward<Futures>(futures)...);
    }

    template<class InputIterator>
    detail::any_future_result_vector<InputIterator>
        when_any(InputIterator first, InputIterator last)
    {
        using namespace std;
        using future_t = typename iterator_traits<InputIterator>::value_type;
        using result_t = when_any_result<vector<future_t>>;

        constexpr auto noIdx = size_t(-1);
        auto indexPromise = make_shared<promise<size_t>>();
        auto indexfuture = indexPromise->get_future();
        vector<future_t> futures;

        size_t index = 0;
        for (auto iterator = first; iterator != last; ++iterator)
        {
            futures.emplace_back(move(*iterator));

            detail::FutureHelper::SetCallback(futures.back(),
                                      [indexPromise, index]()
            {
                detail::FutureHelper::TrySetValue(*indexPromise, index);
            });

            index++;
        }

        if (futures.empty())
            indexPromise->set_value(noIdx);

        return indexfuture.then(
            [sequence = std::move(futures)](auto& firstIndex) mutable
        {
            result_t result;
            result.index = firstIndex.get();
            result.futures = move(sequence);

            return result;
        });
    }

    template<class... Futures>
    detail::any_futures_result_tuple<Futures...>
        when_any(Futures&&... futures)
    {
        using namespace std;
        using result_t = when_any_result<tuple<decay_t<Futures>...>>;

        auto futuresSequence = make_tuple<decay_t<Futures>...>(forward<Futures>(futures)...);
        auto indexPromise = make_shared<promise<size_t>>();
        auto indexfuture = indexPromise->get_future();

        size_t index = 0;
        detail::for_each(futuresSequence, [indexPromise, &index](auto& future) mutable
        {
            detail::FutureHelper::SetCallback(future,
                                      [indexPromise, index]()
            {
                detail::FutureHelper::TrySetValue(*indexPromise, index);
            });

            index++;
        });

        return indexfuture.then(
            [sequence = move(futuresSequence)](auto& firstIndex) mutable
        {
            result_t result;
            result.index = firstIndex.get();
            result.futures = move(sequence);

            return result;
        });
    }
}


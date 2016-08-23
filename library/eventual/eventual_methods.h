#pragma once

#include <cstddef>
#include <tuple>
#include <exception>

#include "detail/detail.h"

namespace eventual
{
    template<class TResult> class future;

    template<class Sequence>
    struct when_any_result
    {
        std::size_t index = std::size_t(-1);
        Sequence futures;
    };

    template<class T>
    future<detail::decay_future_result_t<T>>
        make_ready_future(T&& value);

    inline future<void> make_ready_future();

    template<class T>
    future<T>
        make_exceptional_future(std::exception_ptr ex);

    template<class T, class E>
    future<T>
        make_exceptional_future(E ex);

    template<class InputIterator>
    detail::all_futures_vector<InputIterator>
        when_all(InputIterator first, InputIterator last);

    template<class... Futures>
    detail::all_futures_tuple<Futures...>
        when_all(Futures&&... futures);

    template<class InputIterator>
    detail::any_future_result_vector<InputIterator>
        when_any(InputIterator first, InputIterator last);

    template<class... Futures>
    detail::any_futures_result_tuple<Futures...>
        when_any(Futures&&... futures);

    inline future<when_any_result<std::tuple<>>> when_any();
}

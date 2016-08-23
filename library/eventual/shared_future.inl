#pragma once

#include "shared_future.h"

#include <utility>

#include "detail/BasicFuture.h"
#include "future.h"

#include "shared_future.cpp"

namespace eventual
{
    template<class R>
    shared_future<R>::shared_future(future<R>&& other) noexcept
        : Base(std::forward<future<R>>(other))
    { }

    template<class R>
    shared_future<R>::shared_future(future<shared_future>&& other) noexcept
        : Base(std::forward<future<shared_future>>(other))
    { }

    template<class R>
    template<class F>
    decltype(auto) shared_future<R>::then(F&& continuation)
    {
        return Base::ThenShare(std::forward<F>(continuation), *this);
    }

    template<class R>
    const R& shared_future<R>::get() const
    {
        return Base::GetResultRef<R>();
    }

    template<class R>
    shared_future<R&>::shared_future(future<R&>&& other) noexcept
        : Base(std::forward<future<R&>>(other))
    { }

    template<class R>
    shared_future<R&>::shared_future(future<shared_future>&& other) noexcept
        : Base(std::forward<future<shared_future>>(other))
    { }

    template<class R>
    template<class F>
    decltype(auto) shared_future<R&>::then(F&& continuation)
    {
        return Base::ThenShare(std::forward<F>(continuation), *this);
    }

    template<class R>
    R& shared_future<R&>::get() const
    {
        return Base::GetResultRef<R>();
    }

    template<class F>
    decltype(auto) shared_future<void>::then(F&& continuation)
    {
        return Base::ThenShare(std::forward<F>(continuation), *this);
    }
}

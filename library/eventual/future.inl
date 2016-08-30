#pragma once

#include "future.h"

#include <utility>

#include "detail/utility.h"
#include "detail/BasicFuture.h"
#include "shared_future.h"

#if defined(EVENTUAL_HEADER_ONLY)
#include "future.cpp"
#endif

namespace eventual
{
    template<class RType>
    future<RType>::future(future<future>&& other) noexcept
        : Base(std::forward<future<future>>(other))
    { }

    template<class RType>
    template<class TPromise>
    future<RType>::future(const TPromise& promise, detail::enable_if_not_same_t<future, TPromise, int>)
        : Base(promise)
    { }

    template<class RType>
    template<class F>
    decltype(auto) future<RType>::then(F&& continuation)
    {
        return Base::ThenMove(std::forward<F>(continuation), *this);
    }

    template<class RType>
    RType future<RType>::get()
    {
        return Base::template GetResult<RType>();
    }

    template<class RType>
    shared_future<RType> future<RType>::share()
    {
        return Base::Share(std::move(*this));
    }

    template<class RType>
    future<RType&>::future(future<future>&& other) noexcept
        : Base(std::forward<future<future>>(other))
    { }

    template<class RType>
    template<class TPromise>
    future<RType&>::future(const TPromise& promise, detail::enable_if_not_same_t<future, TPromise, int>)
        : Base(promise)
    { }

    template<class RType>
    template<class F>
    decltype(auto) future<RType&>::then(F&& continuation)
    {
        return Base::ThenMove(std::forward<F>(continuation), *this);
    }

    template<class RType>
    RType& future<RType&>::get()
    {
        return Base::template GetResult<RType&>();
    }

    template<class RType>
    shared_future<RType&> future<RType&>::share()
    {
        return Base::Share(std::move(*this));
    }

    template<class TPromise>
    future<void>::future(const TPromise& promise, detail::enable_if_not_same_t<future, TPromise, int>)
        : Base(promise)
    { }

    template<class F>
    decltype(auto) future<void>::then(F&& continuation)
    {
        return Base::ThenMove(std::forward<F>(continuation), *this);
    }
}


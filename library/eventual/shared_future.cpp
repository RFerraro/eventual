#if defined(EVENTUAL_HEADER_ONLY)
#pragma once
#endif

#include "shared_future.h"

#include <utility>

#include "future.h"
#include "detail/BasicFuture.h"

#if !defined(EVENTUAL_HEADER_ONLY)
#include "eventual.inl"
#endif

namespace eventual
{
    shared_future<void>::shared_future(future<void>&& other) noexcept
        : Base(std::forward<future<void>>(other))
    { }

    shared_future<void>::shared_future(future<shared_future>&& other) noexcept
        : Base(std::forward<future<shared_future>>(other))
    { }

    void shared_future<void>::get() const
    {
        Base::GetResultRef<void>();
    }
}
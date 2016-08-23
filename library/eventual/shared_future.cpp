#pragma once

#include "shared_future.h"

#include <utility>

#include "detail/BasicFuture.inl"
#include "future.inl"

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
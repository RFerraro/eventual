#if defined(EVENTUAL_HEADER_ONLY)
#pragma once
#endif

#include "future.h"

#include <utility>

#include "shared_future.h"

#if !defined(EVENTUAL_HEADER_ONLY)
#include "eventual.inl"
#endif

namespace eventual
{
    void future<void>::get()
    {
        Base::GetResult<void>();
    }

    future<void>::future(future<future>&& other) noexcept
        : Base(std::forward<future<future>>(other))
    { }

    shared_future<void> future<void>::share()
    {
        return Base::Share(std::move(*this));
    }
}

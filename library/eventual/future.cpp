#pragma once

#include "future.h"

#include <utility>

#include "shared_future.h"
#include "detail/utility.h"

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

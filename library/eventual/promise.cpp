#pragma once

#include "promise.inl"
#include "detail/traits.h"

namespace eventual
{
    void promise<void>::swap(promise& other) noexcept
    {
        Base::swap(other);
    }

    void promise<void>::set_value()
    {
        Base::SetValue(detail::Unit());
    }

    void promise<void>::set_value_at_thread_exit()
    {
        Base::SetValueAtThreadExit(detail::Unit());
    }
}
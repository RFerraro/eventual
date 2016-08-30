#if defined(EVENTUAL_HEADER_ONLY)
#pragma once
#endif

#include "promise.h"
#include "detail/traits.h"
#include "detail/BasicPromise.h"
#include "detail/CommonPromise.h"
#include "detail/State.h"

#if !defined(EVENTUAL_HEADER_ONLY)
#include "eventual.inl"
#endif

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
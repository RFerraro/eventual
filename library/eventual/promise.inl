#pragma once

#include "promise.h"

#include <utility>

#include "detail/BasicPromise.inl"

#include "promise.cpp"

namespace std
{
    template<class RType>
    void swap(eventual::promise<RType> &lhs, eventual::promise<RType> &rhs) noexcept
    {
        lhs.swap(rhs);
    }
}

namespace eventual
{
    template<class R>
    template<class Alloc>
    promise<R>::promise(std::allocator_arg_t arg, const Alloc& alloc)
        : Base(arg, alloc)
    { }

    template<class R>
    void promise<R>::swap(promise& other) noexcept
    {
        Base::swap(other);
    }

    template<class R>
    void promise<R>::set_value(const R& value)
    {
        Base::SetValue(value);
    }

    template<class R>
    void promise<R>::set_value(R&& value)
    {
        Base::SetValue(std::forward<R>(value));
    }

    template<class R>
    void promise<R>::set_value_at_thread_exit(const R& value)
    {
        Base::SetValueAtThreadExit(value);
    }

    template<class R>
    void promise<R>::set_value_at_thread_exit(R&& value)
    {
        Base::SetValueAtThreadExit(std::forward<R>(value));
    }

    template<class R>
    template<class Alloc>
    promise<R&>::promise(std::allocator_arg_t arg, const Alloc& alloc)
        : Base(arg, alloc)
    { }

    template<class R>
    void promise<R&>::swap(promise& other) noexcept
    {
        Base::swap(other);
    }

    template<class R>
    void promise<R&>::set_value(R& value)
    {
        Base::SetValue(value);
    }

    template<class R>
    void promise<R&>::set_value_at_thread_exit(R& value)
    {
        Base::SetValueAtThreadExit(value);
    }

    template<class Alloc>
    promise<void>::promise(std::allocator_arg_t arg, const Alloc& alloc)
        : Base(arg, alloc)
    { }
}

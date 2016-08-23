#pragma once

#include "packaged_task.h"

#include <utility>
#include "detail/traits.h"

#include "detail/BasicTask.inl"

namespace std
{
    template<class Function, class... ArgTypes>
    void swap(
        eventual::packaged_task<Function(ArgTypes...)>& lhs,
        eventual::packaged_task<Function(ArgTypes...)>& rhs) noexcept
    {
        lhs.swap(rhs);
    }
}

namespace eventual
{
    template<class R, class... ArgTypes>
    template<class F, class>
    packaged_task<R(ArgTypes...)>::packaged_task(F&& task)
        : Base(detail::decay_copy(std::forward<F>(task)))
    { }

    template<class R, class... ArgTypes>
    template<class F, class Allocator, class>
    packaged_task<R(ArgTypes...)>::packaged_task(std::allocator_arg_t arg, const Allocator& alloc, F&& task)
        : Base(arg, alloc, detail::decay_copy(std::forward<F>(task)))
    { }

    template<class R, class... ArgTypes>
    void packaged_task<R(ArgTypes...)>::swap(packaged_task& other)
    {
        Base::swap(other);
    }
}

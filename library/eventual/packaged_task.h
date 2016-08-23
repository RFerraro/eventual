#pragma once

#include <functional>
#include <memory>

#include "detail/traits.h"
#include "detail/BasicTask.h"

namespace eventual
{
    template <class>
    class packaged_task; // undefined
    
    template<class R, class... ArgTypes>
    class packaged_task<R(ArgTypes...)> : public detail::BasicTask<std::function<R(ArgTypes...)>, R, ArgTypes...>
    {
        using Base = detail::BasicTask<std::function<R(ArgTypes...)>, R, ArgTypes...>;

    public:
        packaged_task() noexcept = default;
        packaged_task(const packaged_task&) = delete;
        packaged_task(packaged_task&& other) noexcept = default;

        template<class F, class = detail::enable_if_not_same<packaged_task, F>>
        explicit packaged_task(F&& task);

        template<class F, class Allocator, class = detail::enable_if_not_same<packaged_task, F>>
        packaged_task(std::allocator_arg_t arg, const Allocator& alloc, F&& task);

        packaged_task& operator=(const packaged_task&) = delete;
        packaged_task& operator=(packaged_task&&) noexcept = default;

        void swap(packaged_task& other);
    };
}

namespace std
{
    template<class RType, class Alloc>
    struct uses_allocator<eventual::packaged_task<RType>, Alloc> : true_type { };

    template<class Function, class... ArgTypes>
    void swap(
        eventual::packaged_task<Function(ArgTypes...)>& lhs,
        eventual::packaged_task<Function(ArgTypes...)>& rhs) noexcept;
}
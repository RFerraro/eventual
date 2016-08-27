#pragma once

#include <memory>

#include "eventual_export.h"
#include "detail/BasicPromise.h"

namespace eventual
{
    template<class R>
    class promise : public detail::BasicPromise<R>
    {
        using Base = detail::BasicPromise<R>;

    public:
        promise() = default;
        promise(promise&& other) noexcept = default;
        promise(const promise& other) = delete;

        template<class Alloc>
        promise(std::allocator_arg_t arg, const Alloc& alloc);

        promise& operator=(const promise& rhs) = delete;
        promise& operator=(promise&& other) noexcept = default;

        void swap(promise& other) noexcept;

        void set_value(const R& value);
        void set_value(R&& value);
        void set_value_at_thread_exit(const R& value);
        void set_value_at_thread_exit(R&& value);
    };

    template<class R>
    class promise<R&> : public detail::BasicPromise<R&>
    {
        using Base = detail::BasicPromise<R&>;

    public:
        promise() = default;
        promise(promise&& other) noexcept = default;
        promise(const promise& other) = delete;

        template<class Alloc>
        promise(std::allocator_arg_t arg, const Alloc& alloc);

        promise& operator=(const promise& rhs) = delete;
        promise& operator=(promise&& other) noexcept = default;

        void swap(promise& other) noexcept;

        void set_value(R& value);
        void set_value_at_thread_exit(R& value);
    };

    template<>
    class promise<void> : public detail::BasicPromise<void>
    {
        using Base = detail::BasicPromise<void>;

    public:
        promise() = default;
        promise(promise&& other) noexcept = default;
        promise(const promise& other) = delete;

        template<class Alloc>
        promise(std::allocator_arg_t arg, const Alloc& alloc);

        promise& operator=(const promise& rhs) = delete;
        promise& operator=(promise&& other) noexcept = default;

        EVENTUAL_API void swap(promise& other) noexcept;

        EVENTUAL_API void set_value();
        EVENTUAL_API void set_value_at_thread_exit();
    };
}

namespace std
{
    template<class RType, class Alloc>
    struct uses_allocator<eventual::promise<RType>, Alloc> : true_type { };

    template<class RType>
    void swap(eventual::promise<RType> &lhs, eventual::promise<RType> &rhs) noexcept;
}
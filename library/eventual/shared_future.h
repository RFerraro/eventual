#pragma once

#include "eventual_export.h"
#include "detail/BasicFuture.h"

namespace eventual
{
    template<class R> class future;
    
    template<class R>
    class shared_future : public detail::BasicFuture<R>
    {
        using Base = detail::BasicFuture<R>;

    public:

        shared_future() noexcept = default;
        shared_future(const shared_future& other) = default;
        shared_future(future<R>&& other) noexcept;
        shared_future(shared_future&& other) noexcept = default;
        shared_future(future<shared_future>&& other) noexcept;

        shared_future& operator=(const shared_future& other) = default;
        shared_future& operator=(shared_future&& other) noexcept = default;

        template<class F>
        decltype(auto) then(F&& continuation);

        const R& get() const;
    };

    template<class R>
    class shared_future<R&> : public detail::BasicFuture<R&>
    {
        using Base = detail::BasicFuture<R&>;

    public:

        shared_future() noexcept = default;
        shared_future(const shared_future& other) = default;
        shared_future(future<R&>&& other) noexcept;
        shared_future(shared_future&& other) noexcept = default;
        shared_future(future<shared_future>&& other) noexcept;

        shared_future& operator=(const shared_future& other) = default;
        shared_future& operator=(shared_future&& other) noexcept = default;

        template<class F>
        decltype(auto) then(F&& continuation);

        R& get() const;
    };

    template<>
    class shared_future<void> : public detail::BasicFuture<void>
    {
        using Base = detail::BasicFuture<void>;

    public:

        shared_future() noexcept = default;
        shared_future(const shared_future& other) = default;
        EVENTUAL_API shared_future(future<void>&& other) noexcept;
        shared_future(shared_future&& other) noexcept = default;

        EVENTUAL_API shared_future(future<shared_future>&& other) noexcept;

        shared_future& operator=(const shared_future& other) = default;
        shared_future& operator=(shared_future&& other) noexcept = default;

        template<class F>
        decltype(auto) then(F&& continuation);

        EVENTUAL_API void get() const;
    };
}

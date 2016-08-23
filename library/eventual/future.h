#pragma once

#include "detail/traits.h"
#include "detail/BasicFuture.h"

namespace eventual
{
    namespace detail
    {
        class FutureFactory;
    }
    
    template<class T> class shared_future;
    
    template<class RType>
    class future : public detail::BasicFuture<RType>
    {
        using Base = detail::BasicFuture<RType>;
        using SharedState = typename Base::SharedState;

        friend class detail::FutureFactory;

    public:
        future() noexcept = default;
        future(future&& other) noexcept = default;
        future(const future& other) = delete;
        future(future<future>&& other) noexcept;

        future& operator=(const future& other) = delete;
        future& operator=(future&& other) noexcept = default;

        template<class F>
        decltype(auto) then(F&& continuation);
        RType get();
        shared_future<RType> share();

    private:

        template<class TPromise>
        future(const TPromise& promise, detail::enable_if_not_same_t<future, TPromise, int> = 0);
    };

    template<class RType>
    class future<RType&> : public detail::BasicFuture<RType&>
    {
        using Base = detail::BasicFuture<RType&>;
        using SharedState = typename Base::SharedState;

        friend class detail::FutureFactory;

    public:
        future() noexcept = default;
        future(future&& other) noexcept = default;
        future(const future& other) = delete;
        future(future<future>&& other) noexcept;

        future& operator=(const future& other) = delete;
        future& operator=(future&& other) noexcept = default;

        template<class F>
        decltype(auto) then(F&& continuation);
        RType& get();
        shared_future<RType&> share();

    private:

        template<class TPromise>
        future(const TPromise& promise, detail::enable_if_not_same_t<future, TPromise, int> = 0);
    };

    template<>
    class future<void> : public detail::BasicFuture<void>
    {
        using Base = detail::BasicFuture<void>;
        using SharedState = typename Base::SharedState;

        friend class detail::FutureFactory;

    public:
        future() noexcept = default;
        future(future&& other) noexcept = default;
        future(const future& other) = delete;
        inline future(future<future>&& other) noexcept;

        future& operator=(const future& other) = delete;
        future& operator=(future&& other) noexcept = default;

        template<class F>
        decltype(auto) then(F&& continuation);
        inline void get();
        inline shared_future<void> share();

    private:

        template<class TPromise>
        future(const TPromise& promise, detail::enable_if_not_same_t<future, TPromise, int> = 0);
    };
}